; Really simple & stupid PICA200 shader
; Taken from picasso example

; Uniforms
.fvec projection[4]
.bool choose_z

; Constants
.constf consts1(0.0, 1.0, 2.0, 0.5)

; Outputs : here only position and color
.out outpos position
.out outclr color

; Inputs : here we have only vertices
.alias inpos v0
.alias incolor v1

.proc main
	; r0 = (inpos.x, inpos.y, inpos.z, 1.0)
	; inpos.w isn't specified by the vertex array, but is implicitly intiialized to 1.0 by the GPU
	mov r0, inpos
	
	; outpos = projection * r1
	dp4 outpos.x, projection[0], r0
	dp4 outpos.y, projection[1], r0
	dp4 outpos.z, projection[2], r0
	dp4 outpos.w, projection[3], r0

	; Set vertex c olor
	mov outclr.xy, incolor
	mov outclr.w, consts1.y

	; incolor.zw aren't specified by the vertex array, but are implicitly initialized to 0.0 and 1.0 by the GPU
	ifu choose_z
		mov outclr.z, incolor.zzzz
	.else
		mov outclr.z, incolor.wwww
	.end

	end
.end
