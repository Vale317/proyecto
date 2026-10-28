#define __SFR_OFFSET 0x00
#include "avr/io.h"

.global init_ports_asm
.global read_adc_ldr_asm
.global read_adc_wind_asm
.global medir_distancia

; =====================================================================
; INICIALIZACIÓN DE PUERTOS Y ADC
; =====================================================================
init_ports_asm:
    ; Trig en D7 (PD7) como SALIDA
    sbi 0x0A, 7         ; DDRD |= (1<<7)
    ; Echo en D6 (PD6) como ENTRADA
    cbi 0x0A, 6         ; DDRD &= ~(1<<6)
    
    ; Inicializar ADC (Habilitar y configurar prescaler)
    ldi r24, 0x40       ; ADMUX = AVCC, Canal 0
    sts 0x7C, r24
    ldi r24, 0x87       ; ADCSRA = Habilitar, Prescaler 128
    sts 0x7A, r24
    ret

; =====================================================================
; LECTURA DEL ADC (LDR en A0)
; =====================================================================
read_adc_ldr_asm:
    ldi r24, 0x40       ; ADMUX = AVCC, Canal 0
    sts 0x7C, r24
    ldi r24, 0x87       ; ADCSRA = Iniciar conversión
    sts 0x7A, r24
wait_adc_ldr:
    lds r24, 0x7A
    sbrc r24, 6         ; Esperar a que ADSC sea 0
    rjmp wait_adc_ldr
    lds r24, 0x78       ; ADCL
    lds r25, 0x79       ; ADCH
    ret

; =====================================================================
; LECTURA DEL ADC (Motor/Viento en A1) - CORREGIDO con Dummy Read
; =====================================================================
read_adc_wind_asm:
    ldi r24, 0x41       ; ADMUX = AVCC, Canal 1
    sts 0x7C, r24
    ldi r24, 0x87       ; Iniciar primera conversión (dummy)
    sts 0x7A, r24
wait_dummy_wind:
    lds r24, 0x7A
    sbrc r24, 6
    rjmp wait_dummy_wind
    
    ldi r24, 0x87       ; Iniciar segunda conversión (real)
    sts 0x7A, r24
wait_adc_wind:
    lds r24, 0x7A
    sbrc r24, 6
    rjmp wait_adc_wind
    lds r24, 0x78       ; ADCL
    lds r25, 0x79       ; ADCH
    ret

; =====================================================================
; MEDIR DISTANCIA (HC-SR04) - CORREGIDO (sbis y límite de división)
; =====================================================================
medir_distancia:
    ; 1. Pulso de 10us en Trig
    sbi 0x0B, 7         ; PORTD |= (1<<7)
    ldi r20, 80
dly_trig:
    dec r20
    brne dly_trig
    cbi 0x0B, 7         ; PORTD &= ~(1<<7)

    ; 2. Esperar a que Echo suba a HIGH (con timeout)
    ldi r26, 0
    ldi r27, 0
wait_echo_high:
    sbis 0x09, 6        ; Si PIND6 es 1, salta a echo_started
    rjmp check_timeout
    rjmp echo_started
check_timeout:
    adiw r26, 1
    cpi r26, 0xFF
    brne wait_echo_high
    cpi r27, 0xFF       ; Timeout más largo (aprox 20ms)
    brne wait_echo_high
    
    ; Timeout alcanzado
    ldi r24, 0
    ldi r25, 0
    ret

    ; 3. Contar tiempo que Echo se mantiene en HIGH (CORREGIDO)
echo_started:
    ldi r24, 0
    ldi r25, 0
count_echo:
    sbis 0x09, 6        ; Si PIND6 es 1, salta el rjmp
    rjmp echo_low       ; Si es 0, salir del ciclo
    adiw r24, 1
    rjmp count_echo

echo_low:
    ; 4. Convertir a cm (Distancia = Tiempo / 58)
    ; CORREGIDO: Revisar si el tiempo es 0 mirando ambos bytes
    cpi r25, 0
    brne hacer_division
    cpi r24, 50
    brlo error_dist
hacer_division:
    ldi r18, 58
    rcall dividir_16_8
    ret
error_dist:
    ldi r24, 0
    ldi r25, 0
    ret

; =====================================================================
; RUTINA DE DIVISIÓN
; =====================================================================
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
