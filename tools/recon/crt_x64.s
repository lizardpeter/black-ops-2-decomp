# x64 (the VS2019 Store build): CRT pieces the Store exe has no callable copy of.
#
# Build environment, not game source.
# - strlen, strcmp: VS2019 expands them inline (intrinsics), so the Store code never calls a CRT copy; strlen isn't
#   even imported. These are the expansions: strcmp compares unsigned bytes and returns -1, 0 or 1.
# - __chkstk: clang calls it for frames over a page; the exe's copy is statically linked and not identified. This is
#   the CRT's routine: probe each page between the stack limit and the new stack pointer (RAX bytes below the
#   caller's), preserving every register but R10/R11, which it restores too.

	.text

	.globl	strlen
	.p2align	4
strlen:
	movq	%rcx, %rax
1:	cmpb	$0, (%rax)
	je	2f
	incq	%rax
	jmp	1b
2:	subq	%rcx, %rax
	retq

	.globl	strcmp
	.p2align	4
strcmp:
1:	movzbl	(%rcx), %eax
	movzbl	(%rdx), %r8d
	cmpl	%r8d, %eax
	jne	2f
	testl	%eax, %eax
	je	3f
	incq	%rcx
	incq	%rdx
	jmp	1b
2:	sbbl	%eax, %eax              # -1 if below
	orl	$1, %eax                # else 1
	retq
3:	xorl	%eax, %eax
	retq

	.globl	__chkstk
	.p2align	4
__chkstk:
	subq	$16, %rsp
	movq	%r10, (%rsp)
	movq	%r11, 8(%rsp)
	xorq	%r11, %r11
	leaq	24(%rsp), %r10
	subq	%rax, %r10
	cmovbq	%r11, %r10
	movq	%gs:16, %r11            # NT_TIB.StackLimit
	cmpq	%r11, %r10
	jae	2f
	andw	$0xF000, %r10w
1:	leaq	-0x1000(%r11), %r11
	movb	$0, (%r11)
	cmpq	%r11, %r10
	jne	1b
2:	movq	(%rsp), %r10
	movq	8(%rsp), %r11
	addq	$16, %rsp
	retq
