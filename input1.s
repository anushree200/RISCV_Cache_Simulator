.data
.dword 20
.dword 30
.dword 40
.dword 50
.dword 60
.text
lui x3, 0x10
sd x0, 0(x3)
ld x4, 8(x3)
ld x4, 0(x3)
ld x4, 16(x3)
ld x4, 24(x3)