; flat assembler interface for Linux
; Copyright (c) 1999-2023, Tomasz Grysztar.
; All rights reserved.

; Hexagonix system interface for flat assembler
; Copyright (c) 2015-2023, Felipe Miguel Nery Lunkes
; All rights reserved.

;;************************************************************************************

use32

;; Now let's create a HAPP header for the application

include "HAPP.s" ;; Here is a structure for the HAPP header

;; Instance | Structure | Architecture | Version | Subversion | Entry Point | Image type
appHeader headerHAPP HAPP.Architectures.i386, 1, 00, fasmX, 01h

;;************************************************************************************

include "hexagon.s" ;; libasm for flat assembler
include "console.s"

;;************************************************************************************

fasmX:

;; Capture parameters provided by Hexagon

    mov [regES], es

    push ds ;; User mode data segment (38h)
    pop es

    mov [command_line], edi

;; Start of the fasm code

    mov esi,_logo
    call    print_string

    call    get_params
    jc  information

    call    init_memory

    mov esi,_memory_prefix
    call    print_string
    mov eax,[memory_end]
    sub eax,[memory_start]
    add eax,[additional_memory_end]
    sub eax,[additional_memory]
    shr eax,10
    call    display_number
    mov esi,_memory_suffix
    call    print_string

    call    getTimeHexagonix
    mov [start_time],eax

    and [preprocessing_done],0
    call    preprocessor
    or  [preprocessing_done],-1
    call    parser
    call    assembler
    call    formatter

    call    display_user_messages
    movzx   eax,[current_pass]
    inc eax
    call    display_number
    mov esi,_passes_suffix
    call    print_string
    call    getTimeHexagonix
    sub eax,[start_time]
    jnc time_ok
    add eax,3600000
      time_ok:
    xor edx,edx
    mov ebx,100
    div ebx
    or  eax,eax
    jz  display_bytes_count
    xor edx,edx
    mov ebx,10
    div ebx
    push    edx
    call    display_number
    mov dl,'.'
    call    display_character
    pop eax
    call    display_number
    mov esi,_seconds_suffix
    call    print_string
      display_bytes_count:
    mov eax,[written_size]
    call    display_number
    mov esi,_bytes_suffix
    call    print_string
    xor al,al
    jmp exit_program

information:
    mov esi,_usage
    call    print_string
    mov al,1
    jmp exit_program

fileSize:     dd 0
filePosition: dd 0
fileName: times 50 db 0

include 'Hexagonix.inc' ;; Interface for Hexagonix

include '../VERSION.INC'

_copyright db 'Copyright (c) 1999-2023, Tomasz Grysztar', 0xA
           db 'Copyright (c) 2015-2025, Felipe Miguel Nery Lunkes', 0xA, 0

_logo db 10, 'flat assembler for Hexagonix version ',VERSION_STRING,0
_usage db 0xA
       db 'Usage: fasmX <source> [output]',0xA
       db 0
_memory_prefix  db '  (',0
_memory_suffix  db ' Kbytes of memory)',0xA,0
_passes_suffix  db ' passes, ',0
_seconds_suffix db ' seconds, ',0
_bytes_suffix   db ' bytes.',0xA,0

include '../ERRORS.INC'
include '../SYMBDUMP.INC'
include '../PREPROCE.INC'
include '../PARSER.INC'
include '../EXPRPARS.INC'
include '../ASSEMBLE.INC'
include '../EXPRCALC.INC'
include '../FORMATS.INC'
include '../X86_64.INC'
include '../AVX.INC'

include '../TABLES.INC'
include '../MESSAGES.INC'

align 4

include '../VARIABLE.INC'

command_line dd ?
memory_setting dd ?
definitions_pointer dd ?
environment dd ?
timestamp dq ?
start_time dd ?
con_handle dd ?
displayed_count dd ?
last_displayed db ?
character db ?
preprocessing_done db ?

predefinitions rb 1000h

regES:  dw 0

endBuffer:
