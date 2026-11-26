param(
    [Parameter(Mandatory)]
    [ValidateSet('mp', 'zm')]
    [string]$Mode
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
$game = Join-Path $env:RUNNER_TEMP "t6-$Mode"
$results = Join-Path (Get-Location) 'audit-results'
$exe = Join-Path $game "t6$Mode.exe"
$serviceExe = (Resolve-Path 'audit-runtime/t6_demonware.exe').Path
$base = 'https://r2.houseofkublai.com/bo2/DUMP'
New-Item -ItemType Directory -Path $game, $results -Force | Out-Null
python tools/fetch_originals.py "t6${Mode}_msstore" $game --only exe
Rename-Item (Join-Path $game "t6${Mode}_msstore.exe") "t6$Mode.exe"
Copy-Item "audit-runtime/$Mode/*.dll" $game

$zones = @('code_pre_gfx', 'code_post_gfx', 'common', 'patch', "code_pre_gfx_$Mode",
    "code_post_gfx_$Mode", "common_$Mode", "patch_$Mode", "ui_$Mode", "patch_ui_$Mode")
$english = @('code_post_gfx', 'common', 'patch', "code_post_gfx_$Mode", "common_$Mode",
    "patch_$Mode", "ui_$Mode", "patch_ui_$Mode")
if ($Mode -eq 'mp') {
    $zones += @('common_patch_mp', 'faction_fbi_mp', 'faction_pmc_mp', 'mp_la')
    $english += @('faction_fbi_mp', 'faction_pmc_mp', 'mp_la')
}
$assets = @('binkw64.dll', 'XCurl.dll', 'MicrosoftGame.config', 'codlogo.bmp',
    'localization_en.txt', 'zone/all/base.ipak', 'zone/english/en_base.ipak')
$assets += $zones | ForEach-Object { "zone/all/$_.ff" }
$assets += $english | ForEach-Object { "zone/english/en_$_.ff" }
foreach ($asset in $assets) {
    $destination = Join-Path $game $asset
    New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
    curl.exe --fail --silent --show-error --location --retry 2 --max-time 600 "$base/$asset" --output $destination
}

function Get-AuditFields($event) {
    $fields = [ordered]@{}
    foreach ($field in ([xml]$event.ToXml()).Event.EventData.Data) {
        if ($field -is [System.Xml.XmlElement]) {
            $fields[$field.GetAttribute('Name')] = $field.InnerText
        }
    }
    return $fields
}

$rule = "T6 outbound audit $Mode"
$trace = Join-Path $env:RUNNER_TEMP "t6-dns-$Mode.etl"
$service = $null
$client = $null
$auditStarted = $false
try {
    # Block direct non-loopback traffic from the game and the local service on the disposable runner.
    auditpol.exe /set '/subcategory:Filtering Platform Connection' /success:enable /failure:enable
    $outsideLoopback = @('0.0.0.0-126.255.255.255', '128.0.0.0-255.255.255.255', '::/0')
    foreach ($program in @($exe, $serviceExe)) {
        New-NetFirewallRule -DisplayName $rule -Direction Outbound -Action Block -Program $program -RemoteAddress $outsideLoopback | Out-Null
    }
    logman.exe start T6DnsAudit -p Microsoft-Windows-DNS-Client 0xffffffffffffffff 5 -o $trace -ets
    $auditStarted = $true
    $begin = Get-Date
    Start-Sleep -Seconds 5

    $serviceLog = Join-Path $results 'service.log'
    $service = Start-Process -FilePath $serviceExe -PassThru -RedirectStandardError $serviceLog
    $deadline = (Get-Date).AddSeconds(15)
    while (-not (Select-String -LiteralPath $serviceLog -Pattern 'auth listening' -Quiet)) {
        if ($service.HasExited -or (Get-Date) -ge $deadline) { throw 'Loopback service did not start' }
        Start-Sleep -Milliseconds 250
    }

    $launched = Get-Date
    $client = Start-Process -FilePath $exe -ArgumentList '--local-demonware' -WorkingDirectory $game -PassThru
    $clientPid = $client.Id
    $deadline = (Get-Date).AddSeconds(150)
    $windowTitle = ''
    while (-not $client.HasExited -and (Get-Date) -lt $deadline) {
        $client.Refresh()
        if ($client.MainWindowTitle) { $windowTitle = $client.MainWindowTitle }
        Start-Sleep -Seconds 2
    }
    $client.Refresh()
    @{
        phase = 'startup and unattended runtime'
        captureStartedUtc = $begin.ToUniversalTime().ToString('o')
        launchedUtc = $launched.ToUniversalTime().ToString('o')
        observedSeconds = [int]((Get-Date) - $launched).TotalSeconds
        windowTitle = $windowTitle
        processId = $clientPid
        exited = $client.HasExited
        exitCode = if ($client.HasExited) { $client.ExitCode } else { $null }
    } | ConvertTo-Json | Set-Content (Join-Path $results 'observation.json')
} finally {
    if ($client -and -not $client.HasExited) {
        [void]$client.CloseMainWindow()
        if (-not $client.WaitForExit(5000)) { $client.Kill(); $client.WaitForExit() }
    }
    if ($service -and -not $service.HasExited) { $service.Kill(); $service.WaitForExit() }
    if ($auditStarted) {
        Start-Sleep -Seconds 5
        $end = Get-Date
        logman.exe stop T6DnsAudit -ets
    }
    Get-NetFirewallRule -DisplayName $rule -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Get-ChildItem $game -Filter 't6_enhanced*.log' | Copy-Item -Destination $results
}

$programs = '\\(t6mp|t6zm|t6_demonware|GamingServices|GamingServicesNet|XboxPcAppFT)\.exe$'
$wfp = @(Get-WinEvent -FilterHashtable @{ LogName = 'Security'; Id = 5156, 5157; StartTime = $begin; EndTime = $end } |
    ForEach-Object {
        $fields = Get-AuditFields $_
        if ($fields.Application -match $programs) {
            [ordered]@{ time = $_.TimeCreated.ToUniversalTime().ToString('o'); eventId = $_.Id; fields = $fields }
        }
    })
ConvertTo-Json -InputObject $wfp -Depth 6 | Set-Content (Join-Path $results 'connections.json')

$vendorDomains = '(demonware\.net|xboxlive\.com|xboxservices\.com|activision\.com|callofduty\.com)'
$dns = @(Get-WinEvent -Path $trace -Oldest | ForEach-Object {
    $fields = Get-AuditFields $_
    if ($_.ProcessId -eq $clientPid -or ($fields.Values -join ' ') -match $vendorDomains) {
        [ordered]@{ time = $_.TimeCreated.ToUniversalTime().ToString('o'); eventId = $_.Id
            processId = $_.ProcessId; fields = $fields }
    }
})
ConvertTo-Json -InputObject $dns -Depth 6 | Set-Content (Join-Path $results 'dns.json')

$loaderActive = Get-ChildItem $results -Filter 't6_enhanced*.log' |
    Select-String -Pattern 'enhanced build active' -Quiet
if (-not $loaderActive) { throw 'The game did not confirm that the fail-closed loader was active' }
