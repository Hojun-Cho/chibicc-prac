# chibicc-prac
[https://github.com/rui314/chibicc](chibicc)

### example

* {int x[2][3]; int *y=x; }

```bash
	.global main
main:
	push %rbp
	mov %rsp, %rbp
	sub $56, %rsp 
	lea -8(%rbp), %rax // y의 주소를 로드
	push %rax  
	lea -56(%rbp), %rax // x의 주소를 y로 로드 
	pop %rdi        
	mov %rax, (%rdi)
.L.return:
	mov %rbp, %rsp
	pop %rbp
	ret
```
