; tapserv.asm
; C64 side server for TAP transfers
; Version 1.4
; (C) 2000-2002 Markus Brenner


; ATN OUT: Bit #3 - indicate sense ('PLAY PRESSED')
; CLK IN:  Bit #6 - data pulse ('receive mode')
; CLK OUT: Bit #4 - data pulse ('send    mode')
; DATA IN: Bit #7 - tapserv mode:
;          0 = send    mode
;          1 = receive mode

DATAIN  = %10000000
CLKIN   = %01000000
DATAOUT = %00100000
CLKOUT  = %00010000
ATNOUT  = %00001000

CASOUT  = %00001000
CASSENSE= %00010000

; Receive Mode: DATAIN = TRUE
SENDMODE= %10000000
RECVMODE= 0


* = $0801

.INCLUDE SYSTEM.ASM			; KERNAL functions

                ; BASIC header "2002 SYS 2062"

		.byte $26,$08,$d2,$07,$9e,$20,$32,$30
                .byte $36,$32,$00,$00,$00


		SEI

		JSR  init_screen
		JSR  init_CIA

                LDA  #$ff
                STA  serv_mode		; invalidate server mode
		STA  sense_out		; invalidate sense_out

		JSR  copy_kernal	; copy kernal
		LDA  $01
                AND  #%11011111		; Motor = ON (Bit 5 = 0)
                AND  #%11111101		; Kernal RAM !
		AND  #%11110111		; Cassette data = 0 
		STA  $01
                STA  $fc		; cassette output byte (0)
		ORA  #CASOUT		; cassette data = 1
		STA  $fd		; cassette output byte (1)
                JSR  init_ints


stopped_loop	LDA  CIA2		; tape is stopped
                AND  #DATAIN		; check for server mode changes
                CMP  serv_mode
		BEQ  slJ1
		STA  serv_mode		; change mode
		JSR  pr_serv_mode	; print new server mode
slJ1		LDA  $01		; check for tape recorder changes
                AND  #CASSENSE		; Sense Bit, 0=play button pressed
                EOR  #CASSENSE
		STA  sense_out
		JSR  screen_color	; Black: tape stopped, White: running
		JSR  set_cia
		LDA  sense_out
		BEQ  stopped_loop

		JSR  screen_off
		JSR  set_ints		; disable/allow interrupts (server mode)
		LDA  serv_mode
                CMP  #RECVMODE
		BEQ  receive_loop
		BNE  send_loop

send_loop	LDA  $01		; PLAY pressed, SEND mode
                AND  #CASSENSE		; Sense Bit, 0=play button down
                EOR  #CASSENSE
		STA  sense_out
		BEQ  sendlJ1

                LDA  CIA2		; check for server mode changes
                AND  #DATAIN
                CMP  #SENDMODE
		BEQ  send_loop
		STA  serv_mode		; change to receive loop
		JSR  pr_serv_mode	; print new server mode
		JSR  set_ints		; disable/allow interrupts (server mode)
		JMP  receive_loop

sendlJ1		SEI			; PLAY released -> state change
		JSR  set_CIA		; clear ATN OUT bit
		JSR  screen_on
		JMP  stopped_loop



receive_loop    LDA  cia_out
                AND  #%11101111		; set CLOCK output to '1'
		STA  cia_out
		STA  CIA2
                LDA  CIA2		; RECORD/PLAY pressed, RECEIVE mode
		STA  last_cia
receiveL1	LDA  last_cia
receiveL2	CMP  CIA2
		BEQ  receiveL2		; wait for change in CIA
		LDA  CIA2
		STA  last_cia
		AND  #CLKIN
                BNE  receiveJ1
		LDA  $fc		; write "0" to tape
                STA  $01
		JMP  receiveJ2
receiveJ1       LDA  $fd		; write "1" to tape
                STA  $01
receiveJ2       JSR  border_cycle
		LDA  last_cia		; check for server mode changes
                AND  #DATAIN
                CMP  #RECVMODE
		BEQ  receiveL1
		STA  serv_mode		; change to send loop
		JSR  pr_serv_mode	; print new server mode
		JSR  set_ints		; disable/allow interrupts (server mode)
		JMP  send_loop


screen_color	LDA  sense_out
		LSR
		LSR
		LSR
		LSR
		STA  $d021
		RTS


my_irq          SEI
                PHA
                LDA  $dc0d		; CIA interrupt control

                LDA  cia_out		; toggle output byte
                EOR  #CLKOUT
                STA  cia_out 
                STA  CIA2

		JSR  border_cycle

                PLA
                RTI

my_nmi		RTI			; direct return from NMI


copy_kernal     LDY  #$00
ckL             LDA  $a000,Y
		STA  $a000,Y
		INY
		BNE  ckL
		INC  ckL+2
		INC  ckL+5
		LDA  ckL+2
		BNE  ckL
		RTS


init_CIA	LDA  CIA2
                AND  #%11000111		; set ATN/DATA/CLOCK outputs to '1'
                STA  cia_out		; init output value
		STA  CIA2		; and set it
                RTS

set_CIA		LDA  cia_out
		AND  #%11110111		; clear ATNOUT
		STA  cia_out
		LDA  sense_out
		LSR
		ORA  cia_out
		STA  cia_out
		STA  CIA2
		RTS


init_ints       LDA  #%00011111		; CLEAR all IRQ interrupts
                STA  $dc0d		; CIA Interrupt control register

                LDA  #%10010000		; SET CASS IRQ interrupt
                STA  $dc0d		; 

                LDA  #<my_irq		; set new irq vector
                STA  $fffe
                LDA  #>my_irq
                STA  $ffff

                LDA  #<my_nmi		; set new nmi vector
                STA  $fffa
                LDA  #>my_nmi
                STA  $fffb

                RTS


set_ints	LDA  serv_mode
                CMP  #RECVMODE
		BEQ  ints_receive
		CLI			; allow   interrupts in send mode
		RTS
ints_receive	SEI			; disable interrupts in receive mode
		RTS


init_screen	JSR  $e544		; clear screen
		JSR  $e566		; home

		LDA  #$0f		; "GREY 3"
		STA  $0286		; COLOR

		LDA  #<screen_text
		LDY  #>screen_text
		JSR  $ab1e		; print string from A/Y
		RTS


pr_serv_mode    JSR  $e9ff		; clear one screen line
		CLC
		LDX  #$0b		; row     1 (0-24)
		LDY  #$00		; column 10 (0-39)
		JSR  PLOT
		LDA  serv_mode
                CMP  #RECVMODE
		BEQ  pr_receive
		LDA  #<send_text
		LDY  #>send_text
		JMP  pr_print
pr_receive	LDA  #<receive_text
		LDY  #>receive_text
pr_print	JSR  $ab1e		; print string from A/Y
		RTS


border_cycle	CLC
                LDA  cycle		; perform color cycle
                ADC  #$01
                AND  #$0f
                STA  cycle
                STA  $d020
		RTS


screen_on       LDA  $d011
		ORA  #$10
		STA  $d011
		RTS

screen_off      LDA  $d011
		AND  #$ef
		STA  $d011
		RTS

serv_mode	.byte $00
sense_out	.byte $00
cia_out 	.byte $00
cycle		.byte $00
last_cia	.byte $00

screen_text	.byte $0d
		.text "          **** TAPSERV 1.4 ****"
		.byte $0d,$0d
		.text "       (C) 2000-2002 MARKUS BRENNER"
		.byte $00

send_text	.text "                SEND MODE "
		.byte $00

receive_text	.text "              RECEIVE MODE"
		.byte $00
