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

function Get-IdentityServices {
    Get-CimInstance -ClassName Win32_Service -Filter @"
Name LIKE 'Xbl%' OR Name LIKE 'Xbox%' OR Name LIKE 'GamingServices%' OR Name LIKE 'GamePlatform%' OR
Name LIKE 'TokenBroker%' OR Name LIKE 'wlidsvc%' OR Name LIKE 'MicrosoftAccount%' OR Name LIKE 'ClipSVC%'
"@ | Select-Object Name, State, ProcessId
}

$rule = "T6 outbound audit $Mode"
$session = "T6ServiceAudit-$Mode"
$trace = Join-Path $env:RUNNER_TEMP "$session.etl"
$env:T6_IDENTITY_AUDIT_FILE = Join-Path $results 'identity-api.log'
$dnsProvider = 'Microsoft-Windows-DNS-Client'
$providers = @(
    'Microsoft-Windows-XboxLive-AuthManager', 'Microsoft-Windows-XboxLive',
    'Microsoft-Windows-XboxLiveCore', 'Microsoft-Windows-Xbox-Identity-Provider',
    'Microsoft-Windows-GamingServices', 'Microsoft-Windows-GamePlatformServices', 'Microsoft-Windows-GameRuntime',
    'Microsoft-Windows-AAD', 'Microsoft-Windows-WebAuth', 'Microsoft-Windows-TokenBroker',
    'Microsoft-Windows-LiveId', 'Microsoft-Windows-WinHttp', 'Microsoft-Windows-WinINet'
)
$service = $null
$client = $null
$auditStarted = $false
$services = [ordered]@{ before = @(); during = @(); after = @() }
$runtimeServices = @{}
$modules = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$observationSeconds = 150
try {
    # Block direct non-loopback traffic from the game and the local service on the disposable runner.
    auditpol.exe /set '/subcategory:Filtering Platform Connection' /success:enable /failure:enable
    $ipv4 = @('1.0.0.0/8', '2.0.0.0/7', '4.0.0.0/6', '8.0.0.0/5', '16.0.0.0/4',
        '32.0.0.0/3', '64.0.0.0/3', '96.0.0.0/4', '112.0.0.0/5', '120.0.0.0/6',
        '124.0.0.0/7', '126.0.0.0/8', '128.0.0.0/2', '192.0.0.0/3', '224.0.0.0/4')
    $ipv6 = @('2000::/3', 'fc00::/7', 'fe80::/10', 'ff00::/8', '64:ff9b::/96', '64:ff9b:1::/48')
    foreach ($program in @($exe, $serviceExe)) {
        foreach ($addresses in @($ipv4, $ipv6)) {
            Write-Host "Installing outbound audit rule for $program ($($addresses[0]))"
            New-NetFirewallRule -DisplayName $rule -Direction Outbound -Action Block -Program $program -RemoteAddress $addresses | Out-Null
        }
    }

    $registeredProviders = @(logman.exe query providers | ForEach-Object {
        if ($_ -match '^\s*(.+?)\s+\{[0-9a-f-]{36}\}\s*$') { $Matches[1] }
    })
    $providers = @($providers + @($registeredProviders | Where-Object {
        $_ -match 'Xbox|XblAuth|GamingServices|GamePlatform|GameRuntime|GameSave|Microsoft-Account|WebAuth|TokenBroker|LiveId'
    }) | Sort-Object -Unique)
    $enabledProviders = @($dnsProvider) + @($providers | Where-Object { $_ -in $registeredProviders })
    @{
        enabled = $enabledProviders
        unavailable = @($providers | Where-Object { $_ -notin $enabledProviders })
    } | ConvertTo-Json | Set-Content (Join-Path $results 'windows-providers.json')
    $providerFile = Join-Path $env:RUNNER_TEMP "$session-providers.txt"
    $enabledProviders | ForEach-Object { "$_ 0xffffffffffffffff 5" } |
        Set-Content -LiteralPath $providerFile -Encoding ascii
    logman.exe start $session -pf $providerFile -o $trace -ets
    $auditStarted = $true
    $begin = Get-Date
    $services.before = @(Get-IdentityServices)
    Start-Sleep -Seconds 5

    $serviceLog = Join-Path $results 'service.log'
    $service = Start-Process -FilePath $serviceExe -PassThru -RedirectStandardError $serviceLog
    $deadline = (Get-Date).AddSeconds(15)
    while (-not (Select-String -LiteralPath $serviceLog -Pattern 'auth listening' -Quiet)) {
        if ($service.HasExited -or (Get-Date) -ge $deadline) { throw 'Loopback service did not start' }
        Start-Sleep -Milliseconds 250
    }

    # Keep the emulator and runner setup the same for an equal-length no-game control.
    $baselineStarted = Get-Date
    $deadline = $baselineStarted.AddSeconds($observationSeconds)
    while ((Get-Date) -lt $deadline) {
        foreach ($identityService in Get-IdentityServices) {
            $key = "$($identityService.Name)/$($identityService.ProcessId)/$($identityService.State)"
            $runtimeServices[$key] = $identityService
        }
        Start-Sleep -Seconds 2
    }
    if ($service.HasExited) { throw 'Loopback service stopped during the no-game control' }

    $launched = Get-Date
    $client = Start-Process -FilePath $exe -ArgumentList '--local-demonware', '--audit-identity' -WorkingDirectory $game -PassThru
    $clientPid = $client.Id
    $deadline = $launched.AddSeconds($observationSeconds)
    $windowTitle = ''
    while (-not $client.HasExited -and (Get-Date) -lt $deadline) {
        $client.Refresh()
        if ($client.MainWindowTitle) { $windowTitle = $client.MainWindowTitle }
        foreach ($module in $client.Modules) {
            [void]$modules.Add($module.ModuleName)
        }
        foreach ($identityService in Get-IdentityServices) {
            $key = "$($identityService.Name)/$($identityService.ProcessId)/$($identityService.State)"
            $runtimeServices[$key] = $identityService
        }
        Start-Sleep -Seconds 2
    }
    $client.Refresh()
    @{
        phase = 'startup and unattended runtime'
        captureStartedUtc = $begin.ToUniversalTime().ToString('o')
        baselineStartedUtc = $baselineStarted.ToUniversalTime().ToString('o')
        baselineSeconds = [int]($launched - $baselineStarted).TotalSeconds
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
        logman.exe stop $session -ets
        $services.after = @(Get-IdentityServices)
    }
    Get-NetFirewallRule -DisplayName $rule -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Get-ChildItem $game -Filter 't6_enhanced*.log' | Copy-Item -Destination $results
}

$services.during = @($runtimeServices.Values | Sort-Object Name, ProcessId, State)
$servicePids = @($services.before + $services.during + $services.after |
    Where-Object ProcessId -gt 0 | Select-Object -ExpandProperty ProcessId -Unique)
ConvertTo-Json -InputObject $services -Depth 5 | Set-Content (Join-Path $results 'windows-identity-services.json')
$sampledModules = @($modules | Sort-Object)
ConvertTo-Json -InputObject $sampledModules | Set-Content (Join-Path $results 'sampled-game-modules.json')
$identityModules = @($sampledModules | Where-Object {
    $_ -match 'xbox|xgameruntime|gamingservices|gameplatform|tokenbroker|wlid|webauth|winhttp|wininet|xcurl'
})
ConvertTo-Json -InputObject $identityModules | Set-Content (Join-Path $results 'identity-modules.json')

$programs = '\\(t6mp|t6zm|t6_demonware|GamingServices\w*|GamePlatform\w*|Xbox\w*|GameBar\w*)\.exe$'
$wfp = @(try {
    Get-WinEvent -FilterHashtable @{ LogName = 'Security'; Id = 5156, 5157; StartTime = $begin; EndTime = $end } | ForEach-Object {
        $fields = Get-AuditFields $_
        if ($fields.Application -match $programs -or $fields.ProcessID -in $servicePids) {
            [ordered]@{ time = $_.TimeCreated.ToUniversalTime().ToString('o'); eventId = $_.Id; fields = $fields }
        }
    }
} catch {
    if ($_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound,*') { throw }
})
ConvertTo-Json -InputObject $wfp -Depth 6 | Set-Content (Join-Path $results 'connections.json')

$vendorDomains = '(demonware\.net|xboxlive\.com|xboxservices\.com|xbox\.com|gamingservices\.microsoft\.com|login\.live\.com|login\.microsoftonline\.com|msauth\.net|msftauth\.net|activision\.com|callofduty\.com)'
$dns = [System.Collections.Generic.List[object]]::new()
$authEvents = [System.Collections.Generic.List[object]]::new()
Get-WinEvent -Path $trace -Oldest | ForEach-Object {
    $entry = [ordered]@{ time = $_.TimeCreated.ToUniversalTime().ToString('o'); eventId = $_.Id
        processId = $_.ProcessId; provider = $_.ProviderName }
    if ($_.ProviderName -eq $dnsProvider) {
        $fields = Get-AuditFields $_
        if ($_.ProcessId -eq $clientPid -or $_.ProcessId -in $servicePids -or ($fields.Values -join ' ') -match $vendorDomains) {
            $entry.fields = $fields
            $dns.Add($entry)
        }
    } elseif ($_.ProviderName -notin @('Microsoft-Windows-WinHttp', 'Microsoft-Windows-WinINet') -or
        $_.ProcessId -eq $clientPid -or $_.ProcessId -in $servicePids) {
        # Event metadata identifies providers and processes without exposing account data.
        $entry.activityId = $_.ActivityId
        $entry.relatedActivityId = $_.RelatedActivityId
        $authEvents.Add($entry)
    }
}
ConvertTo-Json -InputObject $dns -Depth 6 | Set-Content (Join-Path $results 'dns.json')
ConvertTo-Json -InputObject $authEvents -Depth 5 | Set-Content (Join-Path $results 'windows-auth-events.json')

Write-Host "Captured $($wfp.Count) connection events and $($dns.Count) DNS events in scope."
$authEvents | Group-Object { $_.provider } | ForEach-Object { Write-Host "$($_.Name) emitted $($_.Count) events." }
Get-Content (Join-Path $results 'observation.json') | Write-Host
$identityModules | ForEach-Object { Write-Host "Observed module $_" }
if (Test-Path $env:T6_IDENTITY_AUDIT_FILE) {
    Get-Content $env:T6_IDENTITY_AUDIT_FILE | Group-Object | ForEach-Object {
        Write-Host "$($_.Name) ($($_.Count) events)"
    }
}

$loaderActive = Get-ChildItem $results -Filter 't6_enhanced*.log' |
    Select-String -Pattern 'enhanced build active' -Quiet
if (-not $loaderActive) { throw 'The game did not confirm that the fail-closed loader was active' }
if (-not (Test-Path $env:T6_IDENTITY_AUDIT_FILE)) { throw 'The game did not reach the instrumented GDK loader' }
