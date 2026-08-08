; flat assembler interface for Linux
; Copyright (c) 1999-2023, Tomasz Grysztar.
; All rights reserved.

; Hexagonix system interface for flat assembler
; Copyright (c) 2015-2023, Felipe Miguel Nery Lunkes
; All rights reserved.

;;************************************************************************************
;;
;; Interface for running on Hexagonix
;;
;;************************************************************************************

;; Input:
;;
;; [command_line] = Points to the line provided by the system
;;
;; Output:
;;
;; [input_file] - Source code name
;; [output_file] - Name of the binary to be generated
;;
;; CF set in case of error

getParameters:

    mov esi, [command_line]
    mov [input_file], esi
        
    cmp byte[esi], 0
    je .invalidParameters
    
    mov [symbols_file], 0
    mov [memory_setting], 0
    
    mov [passes_limit], 100
    
    mov al, ' '

    hx.syscall hx.findCharacter

    jc .singleArgument

    mov al, ' '

    call findString
    
    mov [output_file], esi
    jmp .done
    
.singleArgument:

    mov [output_file], 0

.done:

    clc

    ret
    
.invalidParameters:

    stc

    ret

;;************************************************************************************

;; Search for a specific character in a string
;;
;; Input:
;;
;; ESI - String
;; AL - Character to search
;;
;; Output:
;;
;; ESI - Character position

findString:

    lodsb
    
    cmp al, ' '
    je .done
    
    jmp findString

.done:

    mov byte[esi-1], 0

    ret

;;************************************************************************************

finishProcess:

    mov es, [regES]
    
    mov ebx, 00h
    
    hx.syscall hx.exit

    ret

;;************************************************************************************

;; Input:
;;
;; ESI - String

invalidParameters:

    push ebx

    printString

    pop ebx

    ret

;;************************************************************************************

;; Input:
;;
;; AL - Character

displayCharacter:

    push ebx

    mov al, dl

    hx.syscall hx.printCharacter

    pop ebx

    ret

;;************************************************************************************

;; Input:
;;
;; EAX - Number

displayNumber:

    push ebx

    printInteger

    pop ebx

    ret

;;************************************************************************************

;; Input:
;;
;; ESI - String
;; ECX - Characters to display

displayBlock:

    push ebx

.loop:

    lodsb

    hx.syscall hx.printCharacter

    loop .loop
    
    pop ebx

    ret

;;************************************************************************************

;; Input:
;;
;; EDX - Pointer to filename
;; EBX - File handler (ignored)
;;
;; Output:
;;
;; CF defined when file does not exist

openFileHexagonix:

    pushad
    
    push es 

    push ds 
    pop es

    mov esi, edx

    hx.syscall hx.stringSize

    inc eax
    
    mov ecx, eax
    mov esi, edx
    mov edi, fileName

    rep movsb
    
    pop es 

    popad
    pushad
        
    mov esi, fileName
    mov edi, [file_buffer]

    xor ecx, ecx

    hx.syscall hx.open

    jc .error

    mov [fileSize], eax
    
    clc

    popad

    mov ebx, 3

    ret

.error:

    stc

    popad

    ret

;;************************************************************************************

;; Input:
;;
;; EDX - Pointer to filename
;; EBX - File handler (ignored)
;;
;; Output:
;;
;; CF defined in case of error

createFileHexagonix:

    pushad
    
    push es 

    push ds
    pop ds

    mov esi, edx

    hx.syscall hx.stringSize

    inc eax
    
    mov ecx, eax
    mov esi, edx
    mov edi, fileName

    rep movsb
    
    pop es

    mov esi, fileName

    hx.syscall hx.unlink
    
    popad
    pushad
    
    mov eax, 0
    mov esi, fileName
    mov edi, fileName

    hx.syscall hx.create        

    jnc .success       
    
.error:

    mov al, 'C'

    hx.syscall hx.printCharacter

    stc

    jmp .end
    
.success:

    clc

.end:   

    popad

    mov ebx, 3

    ret

;;************************************************************************************

;; Input:
;;
;; EBX - File handler (ignored)
;; ECX - Number of bytes to read
;; EDX - Buffer to read
;;
;; Output:
;;
;; CF defined in case of error

readFileHexagonix:

    pushad

    push es

    push ds
    pop es
    
    mov esi, [file_buffer]

    add esi, [filePosition]

    mov edi, edx

    rep movsb

    pop es
    
    popad

    clc

    ret

;;************************************************************************************

;; Input:
;;
;; EBX - File handler (ignored)
;; ECX - Bytes to read
;; EDX - Data to read
;;
;; Output:
;;
;; CF defined in case of error

writeFileHexagonix:

    mov esi, fileName

    hx.syscall hx.unlink
    
    mov eax, ecx
    mov edi, edx
    mov esi, fileName

    hx.syscall hx.create

    jc .error

    clc

    ret

.error:

    ret

;;************************************************************************************

;; Move to certain position in the file
;;
;; Input:
;;
;; EDX - Bytes to skip
;; AL - Origin (0 = start, 1 = current position, 2 = end)
;;
;; Output:
;;
;; EAX - Position in file

fileSeekHexagonix:

    pushad

    cmp al, 0   
    je .fromBeginning

    cmp al, 1   
    je .currentPosition

    cmp al, 2
    je .fromEnd

    jmp .end
    
.fromBeginning:  

    mov [filePosition], edx

    jmp .end

.currentPosition:

    add [filePosition], edx

    jmp .end
    
.fromEnd:

    mov eax, [fileSize]
    mov [filePosition], eax

    add [filePosition], edx

.end:

    popad

    mov eax, [filePosition]
    
    ret

;;************************************************************************************

;; Returns an approximate monotonic millisecond timestamp, used only to
;; measure elapsed assembly time between two readings taken close
;; together. hx.time returns the real-time clock's hour/minute/second as
;; raw BCD bytes (not true ASCII, despite the syscall's name), so each
;; one is converted to binary before combining into milliseconds since
;; midnight
;;
;; Output:
;;
;; EAX - Milliseconds since midnight

getTimeHexagonix:

    push ebx
    push ecx
    push edx

    hx.syscall hx.time ;; EAX = hour (BCD), EBX = minute (BCD), ECX = second (BCD)

    call .toBinary ;; EAX = hour, binary

    mov [.hour], eax

    mov eax, ebx

    call .toBinary

    mov [.minute], eax

    mov eax, ecx

    call .toBinary

    mov [.second], eax

    mov eax, [.hour]
    mov ebx, 3600

    mul ebx

    mov ecx, eax ;; ECX = hour * 3600 seconds

    mov eax, [.minute]
    mov ebx, 60

    mul ebx

    add ecx, eax ;; ECX += minute * 60 seconds

    add ecx, [.second] ;; ECX = total seconds since midnight

    mov eax, ecx
    mov ebx, 1000

    mul ebx ;; EAX = milliseconds since midnight

    pop edx
    pop ecx
    pop ebx

    ret

.toBinary:

;; Input:
;;
;; AL - BCD byte (tens digit in the high nibble, units in the low nibble)
;;
;; Output:
;;
;; EAX - Binary value

    push ebx

    movzx ebx, al

    mov eax, ebx
    shr eax, 4

    imul eax, 10 ;; Tens digit

    and ebx, 0Fh ;; Units digit

    add eax, ebx

    pop ebx

    ret

.hour:   dd 0
.minute: dd 0
.second: dd 0

;;************************************************************************************

;; Address the source file is loaded into by openFileHexagonix, set once
;; init_memory has actually allocated the memory backing it

file_buffer dd 0

;;************************************************************************************

;;************************************************************************************
;;
;; End of specific implementation for Hexagonix
;;
;;************************************************************************************

; flat assembler
; Copyright (c) 1999-2023, Tomasz Grysztar.
; All rights reserved.

; Hexagonix system interface for flat assembler
; Copyright (c) 2015-2025, Felipe Miguel Nery Lunkes
; All rights reserved.

;; Redirection of functions and adaptations to run on Hexagonix

get_params:

    jmp getParameters

;;************************************************************************************

findChar:

    jmp findString

;;************************************************************************************

get_environment_variable:

    ret

make_timestamp:

    ret

;;************************************************************************************

exit_program:

    jmp finishProcess

;;************************************************************************************

print_string:

    jmp invalidParameters

;;************************************************************************************

display_character:

    jmp displayCharacter

;;************************************************************************************

display_number:

    jmp displayNumber

;;************************************************************************************

display_block:

    jmp displayBlock

;;************************************************************************************

;________________________________________________
;Initialize memory
;IN:    nothing
;OUT:   [memory_start]  = Start of free main memory block
;   [memory_end]    = End of free main memory block
;   [additional_memory]     = Start of additional memory
;   [additional_memory_end] = End of additional memory
;

init_memory:

    mov eax, esp
    add eax, 1000h-10000h

    mov [stack_limit], eax

;; fasmX (ported from Linux) expects a large scratch region past its own
;; code for its own symbol tables, tokenized source and the like, plus
;; somewhere to load the source file being assembled into
;; (openFileHexagonix/readFileHexagonix, right past additional_memory_end).
;; Hexagonix only reserves a small fixed margin past a process's own
;; image (see Hexagon.Kern.Proc.allocateAndLoadImage), nowhere near
;; enough, so all of this has to be requested from the kernel explicitly
;; rather than assumed free just past endBuffer

    mov ebx, 1024*13000 + 1024*4000 ;; Scratch region, plus room for the source file

    hx.syscall hx.malloc

    mov [memory_start], ebx

    mov eax, ebx
    add eax, 1024*10000

    mov [memory_end], eax
    mov [additional_memory], eax

    add eax, 1024*3000

    mov [additional_memory_end], eax
    mov [file_buffer], eax

    ret

;;************************************************************************************

open:

    jmp openFileHexagonix

;;************************************************************************************

create:

    jmp createFileHexagonix

;;************************************************************************************

close:

    stc

    ret

;;************************************************************************************

read:

    jmp readFileHexagonix

;;************************************************************************************

write:

    jmp writeFileHexagonix

;;************************************************************************************

lseek:

    jmp fileSeekHexagonix

;;************************************************************************************

;________________________________________________
;Display error and sair
;IN:    Pointer to error message on stack
;OUT:   nothing
;

fatal_error:

    mov esi, error_prefix
    call print_string
    
    pop esi ;Error message 
    call print_string
    
    mov esi, error_suffix
    call print_string
    
    jmp exit_program

;________________________________________________
;Display user messages
;IN/OUT: nothing
;   
display_user_messages:
    mov [displayed_count],0
    call show_display_buffer
    
    cmp [displayed_count],0
    je .line_break_ok
    
    cmp [last_displayed],0Ah
    je .line_break_ok
    
    mov dl,0Ah
    call display_character
    
.line_break_ok:
    ret
    
;________________________________________________
;Display assembler error and sair
;IN/OUT: see fasmguide.txt
;   
assembler_error:
    call    display_user_messages
    push    dword 0
    mov ebx,[current_line]
      get_error_lines:
    mov eax,[ebx]
    cmp byte [eax],0
    je  get_next_error_line
    push    ebx
    test    byte [ebx+7],80h
    jz  display_error_line
    mov edx,ebx
      find_definition_origin:
    mov edx,[edx+12]
    test    byte [edx+7],80h
    jnz find_definition_origin
    push    edx
      get_next_error_line:
    mov ebx,[ebx+8]
    jmp get_error_lines
      display_error_line:
    mov esi,[ebx]
    call    print_string
    mov esi,line_number_start
    call    print_string
    mov eax,[ebx+4]
    and eax,7FFFFFFFh
    call    display_number
    mov dl,']'
    call    display_character
    pop esi
    cmp ebx,esi
    je  line_number_ok
    mov dl,20h
    call    display_character
    push    esi
    mov esi,[esi]
    movzx   ecx,byte [esi]
    inc esi
    call    display_block
    mov esi,line_number_start
    call    print_string
    pop esi
    mov eax,[esi+4]
    and eax,7FFFFFFFh
    call    display_number
    mov dl,']'
    call    display_character
      line_number_ok:
    mov esi,line_data_start
    call    print_string
    mov esi,ebx
    mov edx,[esi]
    call    open
    mov al,2
    xor edx,edx
    call    lseek
    mov edx,[esi+8]
    sub eax,edx
    push    eax
    xor al,al
    call    lseek
    mov ecx,[esp]
    mov edx,[additional_memory]
    lea eax,[edx+ecx]
    cmp eax,[additional_memory_end]
    ja  out_of_memory
    call    read
    call    close
    pop ecx
    mov esi,[additional_memory]
      get_line_data:
    mov al,[esi]
    cmp al,0Ah
    je  display_line_data
    cmp al,0Dh
    je  display_line_data
    cmp al,1Ah
    je  display_line_data
    or  al,al
    jz  display_line_data
    inc esi
    loop    get_line_data
      display_line_data:
    mov ecx,esi
    mov esi,[additional_memory]
    sub ecx,esi
    call    display_block
    mov esi,lf
    call    print_string
    pop ebx
    or  ebx,ebx
    jnz display_error_line
    mov esi,error_prefix
    call    print_string
    pop esi
    call    print_string
    mov esi,error_suffix
    call    print_string
    mov al,2
    jmp exit_program
;______________________________________________________________________________

error_prefix db 'Error: ',0
error_suffix db '.'
lf db 0xA,0
line_number_start db ' [',0
line_data_start db ':',0xA,0