gdb ./bomb
b *0x0000000000400dd1
r
set $rip = 0x0000000000400e31
b *0x0000000000400e52
c
set $rip = 0x0000000000400e57
b *0x0000000000400e6e
c
set $rip = 0x0000000000400e73
b *0x0000000000400e8a
c
set $rip = 0x0000000000400e8f
b *0x0000000000400ea6
c
set $rip = 0x0000000000400eab
b *0x0000000000400ec2
c
set $rip = 0x0000000000400ec7
b *0x0000000000400ede
c

Public speaking is very easy.
1 2 4 8 16 32 
0 374 (case 잘 나눠서)
6 21 SecretPhase (내부 함수의 처리, 이분 탐색등 설명))
GAMFEG (&0xf 한 array의 index)
6 5 2 4 3 1
22

secret_phase

SecretPhase

22 입력하면 됨. 트리 구조 잘 그리기

zaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaazzaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaazzaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaazzaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaz