# runtime/loader/xinput_stubs.s: XINPUT1_4.dll's exports, each a jump through its slot to the system DLL's function
# (filled by LoadSystemXInput), so every argument passes through untouched whatever the signature.
	.text
	.globl XInputGetState
XInputGetState: jmpq *xinput_slots+0(%rip)
	.globl XInputSetState
XInputSetState: jmpq *xinput_slots+8(%rip)
	.globl XInputGetCapabilities
XInputGetCapabilities: jmpq *xinput_slots+16(%rip)
	.globl XInputEnable
XInputEnable: jmpq *xinput_slots+24(%rip)
	.globl XInputGetBatteryInformation
XInputGetBatteryInformation: jmpq *xinput_slots+32(%rip)
	.globl XInputGetKeystroke
XInputGetKeystroke: jmpq *xinput_slots+40(%rip)
	.globl XInputGetAudioDeviceIds
XInputGetAudioDeviceIds: jmpq *xinput_slots+48(%rip)
