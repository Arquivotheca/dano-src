	.file	"ftol.s"
.text
	.align 16
.globl _ftol
	.type	 _ftol,@function
_ftol:
	subl $12,%esp
	fnstcw 8(%esp)
	movl 8(%esp),%edx
	movb $12,%dh
	movl %edx,(%esp)
	fldcw (%esp)
	fistpl (%esp)
	movl (%esp),%eax
	fldcw 8(%esp)
	addl $12,%esp
	ret
