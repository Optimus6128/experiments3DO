; void vramCpy(void* bufferSrc, void* bufferDst, int length)

    AREA |C$$code|, CODE, READONLY
|x$codeseg|

    EXPORT vramCpy

vramCpy
	stmfd sp!, {r4-r11, lr}

loopVramCpy
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
		ldmia r0!,{r4-r11}
		stmia r1!,{r4-r11}
	subs r2,r2,#256
	bgt loopVramCpy

	ldmfd sp!, {r4-r11, pc}

	END
