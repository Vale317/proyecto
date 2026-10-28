; AVR.S - Estacion meteorologica
#define __SFR_OFFSET 0
#include <avr/io.h>

.equ PIN_LED,   4        ; PB4 (D12)
.equ PIN_TRIG,  7        ; PD7 (D7)
.equ PIN_ECHO,  6        ; PD6 (D6)

.equ LIM_INICIO_ECHO, 30000   ; iteraciones de 6 ciclos (~11 ms) esperando que Echo suba
.equ LIM_ECHO_US,     25000   ; ancho maximo de eco (us), ~430 cm
.equ DIST_ERROR,      0xFFFF

.global init_ports_asm
.global leer_adc
.global escalar_adc
.global medir_distancia
.global led_alerta

; init_ports_asm : pines digitales y ADC
init_ports_asm:
    SBI  DDRD,  PIN_TRIG      ; Trig salida
    CBI  PORTD, PIN_TRIG      ; Trig en bajo
    CBI  DDRD,  PIN_ECHO      ; Echo entrada
    SBI  DDRB,  PIN_LED       ; LED alerta salida
    CBI  PORTB, PIN_LED       ; apagado
    LDI  R18, (1<<REFS0)                                  ; AVcc, canal 0
    STS  ADMUX, R18
    LDI  R18, (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0) ; ADC on, prescaler 128
    STS  ADCSRA, R18
    RET

; uint16_t leer_adc(uint8_t canal)     canal en R24 (0 = A0, 1 = A1, ...)
; Retorna 10 bits en R25:R24. La primera conversion se descarta
; (asentamiento del multiplexor al cambiar de canal).
leer_adc:
    ANDI R24, 0x0F
    ORI  R24, (1<<REFS0)
    STS  ADMUX, R24
    LDI  R19, 2
leer_adc_conv:
    LDI  R18, (1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0) ; 0xC7: inicia conversion
    STS  ADCSRA, R18
leer_adc_espera:
    LDS  R18, ADCSRA
    SBRC R18, ADSC            ; esperar a que ADSC sea 0
    RJMP leer_adc_espera
    DEC  R19
    BRNE leer_adc_conv
    LDS  R24, ADCL            ; ADCL siempre antes que ADCH
    LDS  R25, ADCH
    RET

; uint16_t escalar_adc(uint16_t raw, uint8_t k)   raw = R25:R24, k = R22
; Retorna (raw * k) >> 8
escalar_adc:
    MUL  R24, R22
    MOV  R18, R1
    MUL  R25, R22
    ADD  R18, R0
    CLR  R19
    ADC  R1, R19
    MOV  R24, R18
    MOV  R25, R1
    CLR  R1                   ; R1 debe quedar en 0
    RET

; uint16_t medir_distancia(void)
; Retorna cm en R25:R24, o 0xFFFF si no hubo eco / fuera de rango.
; Cada vuelta del lazo de conteo dura 16 ciclos = 1 us.
medir_distancia:
    SBI  PORTD, PIN_TRIG      ; pulso de disparo (>= 10 us)
    LDI  R20, 80              ; 80 * 3 ciclos = 15 us
dist_trig:
    DEC  R20
    BRNE dist_trig
    CBI  PORTD, PIN_TRIG

    LDI  R26, lo8(LIM_INICIO_ECHO)
    LDI  R27, hi8(LIM_INICIO_ECHO)
dist_espera_eco:
    SBIC PIND, PIN_ECHO       ; Echo en BAJO -> salta
    RJMP dist_eco_alto        ; Echo en ALTO -> empezar a contar
    SBIW R26, 1
    BRNE dist_espera_eco
    RJMP dist_timeout

dist_eco_alto:
    CLR  R24
    CLR  R25
    LDI  R22, lo8(LIM_ECHO_US)
    LDI  R23, hi8(LIM_ECHO_US)
dist_cuenta:                  ; 2+2+1+1+1+1+6+2 = 16 ciclos
    SBIS PIND, PIN_ECHO       ; 2  Echo en ALTO -> salta
    RJMP dist_fin_eco
    ADIW R24, 1               ; 2
    CP   R24, R22             ; 1
    CPC  R25, R23             ; 1
    BRSH dist_timeout         ; 1
    NOP                       ; 1
    RJMP .+0                  ; 2
    RJMP .+0                  ; 2
    RJMP .+0                  ; 2
    RJMP dist_cuenta          ; 2

dist_fin_eco:
    LDI  R18, 58              ; cm = us / 58
    RJMP dividir_16_8         ; vuelve al llamador con el resultado en R25:R24

dist_timeout:
    LDI  R24, 0xFF
    LDI  R25, 0xFF
    RET

; dividir_16_8 (interna): R25:R24 / R18 -> cociente R25:R24, resto R21
dividir_16_8:
    CLR  R21
    LDI  R20, 16
div_loop:
    LSL  R24
    ROL  R25
    ROL  R21
    CP   R21, R18
    BRLO div_siguiente
    SUB  R21, R18
    ORI  R24, 1
div_siguiente:
    DEC  R20
    BRNE div_loop
    RET

; void led_alerta(uint8_t encendido)    R24 != 0 enciende el LED (D12)
led_alerta:
    TST  R24
    BREQ led_apagar
    SBI  PORTB, PIN_LED
    RET
led_apagar:
    CBI  PORTB, PIN_LED
    RET
