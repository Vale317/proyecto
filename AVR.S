#define __SFR_OFFSET 0x00
#include "avr/io.h"

.global init_ports_asm
.global read_adc_ldr_asm
.global read_adc_wind_asm
.global medir_distancia

; INICIALIZACIÓN DE PUERTOS

init_ports_asm:
    ; Trig en D7 (PD7) como SALIDA
    sbi 0x0A, 7         ; DDRD |= (1<<7)
    
    ; Echo en D6 (PD6) como ENTRADA
    cbi 0x0A, 6         ; DDRD &= ~(1<<6)
    
    ; Los pines A0 y A1 por defecto ya son entradas analógicas.
    ret

; LECTURA DEL ADC (LDR en A0)
; Retorna el valor de 10 bits en R25:R24 (R25=High, R24=Low)

read_adc_ldr_asm:
    ; ADMUX = 0x40 (Referencia AVCC, Canal 0)
    ldi r24, 0x40
    sts 0x7C, r24
    ; ADCSRA = 0x87 (Habilitar ADC, Prescaler 128)
    ldi r24, 0x87
    sts 0x7A, r24
wait_adc_ldr:
    lds r24, 0x7A
    sbrc r24, 6         ; Esperar a que ADSC (bit 6) sea 0
    rjmp wait_adc_ldr
    ; Leer resultado
    lds r24, 0x78       ; ADCL
    lds r25, 0x79       ; ADCH
    ret

; LECTURA DEL ADC (Motor/Viento en A1)
; Retorna el valor de 10 bits en R25:R24

read_adc_wind_asm:
    ; ADMUX = 0x41 (Referencia AVCC, Canal 1)
    ldi r24, 0x41
    sts 0x7C, r24
    ; ADCSRA = 0x87 (Habilitar ADC, Prescaler 128)
    ldi r24, 0x87
    sts 0x7A, r24
wait_adc_wind:
    lds r24, 0x7A
    sbrc r24, 6
    rjmp wait_adc_wind
    ; Leer resultado
    lds r24, 0x78
    lds r25, 0x79
    ret

; MEDIR DISTANCIA (HC-SR04)
; Trig en D7 (PD7), Echo en D6 (PD6)
; Retorna la distancia en cm en R25:R24. Si hay error, retorna 0.

medir_distancia:
    ; 1. Enviar pulso de 10us en Trig (PD7)
    sbi 0x0B, 7         ; PORTD |= (1<<7)
    ldi r20, 80
dly_trig:
    dec r20
    brne dly_trig
    cbi 0x0B, 7         ; PORTD &= ~(1<<7)

    ; 2. Esperar a que Echo (PD6) suba a HIGH (con timeout)
    ldi r26, 0          ; Contador de timeout Low
    ldi r27, 0          ; Contador de timeout High
wait_echo_high:
    sbis 0x09, 6        ; Si PINB (0x09), bit 6 (Echo) es 1, salta
    rjmp check_timeout
    rjmp echo_started
check_timeout:
    adiw r26, 1         ; Incrementa el par r27:r26
    cpi r26, 0xFF
    brne wait_echo_high
    cpi r27, 0x10       ; Límite de espera
    brne wait_echo_high
    ; Si llega aquí, no hubo eco (Timeout)
    ldi r24, 0
    ldi r25, 0
    ret

    ; 3. Contar tiempo que Echo se mantiene en HIGH
echo_started:
    ldi r24, 0
    ldi r25, 0
count_echo:
    sbic 0x09, 6        ; Si PINB bit 6 es 0, salta a echo_low
    rjmp echo_low
    adiw r24, 1         ; Incrementa el par r25:r24
    rjmp count_echo

echo_low:
    ; 4. Convertir tiempo a cm (Distancia = Tiempo / 58)
    cpi r24, 50
    brlo error_dist     ; Si el tiempo es muy bajo, dar 0
    ldi r18, 58
    rcall dividir_16_8
    ret
error_dist:
    ldi r24, 0
    ldi r25, 0
    ret
    
; RUTINA DE DIVISIÓN (R25:R24 / R18)

dividir_16_8:
    ldi r20, 16
    clr r21
div_loop:
    lsl r24
    rol r25
    rol r21
    sub r21, r18
    brcc div_ok
    add r21, r18
    clc
    rjmp div_next
div_ok:
    sec
div_next:
    rol r24
    rol r25
    dec r20
    brne div_loop
    ret
