; void vramCpy(void* bufferSrc, void* bufferDst)

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    EXPORT vramCpy

vramCpy
	stmfd sp!, {r4-r11, lr}

	mov r2,r0
	mov r3,r1

	mov r1,#600
loopVramCpy
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
		ldmia r2!,{r4-r11}
		stmia r3!,{r4-r11}
	subs r1,r1,#1
	bne loopVramCpy

	ldmfd sp!, {r4-r11, pc}

	END
