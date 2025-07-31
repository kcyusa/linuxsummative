; ===============================
; Program: Count Names in File
; Author: Cyusa Keny
; Description:
;   This program reads a text file (same directory), counts the number of lines
;   (including empty lines), and prints the total count to the console.
; ===============================

; ===== File Handling Approach =====
; 1. Open file using sys_open (read-only).
; 2. Read entire file content into a memory buffer using sys_read.
; 3. Close file after reading.
; 4. Traverse the buffer and count lines (newline characters '\n').
; 5. Print the count as an integer using sys_write.

; ===== String Traversal and Line Counting =====
; - The file is read into a buffer.
; - Loop through each byte:
;       - If character is '\n', increment counter.
; - After loop, if file doesn't end with '\n', increment counter for last line.

; ===== Control Flow =====
; - main -> open file -> read file -> count lines -> convert count to string -> print -> exit.
; - Loop:
;       mov al, [rsi]          ; get current character
;       cmp al, 0              ; check end of buffer
;       je  done               ; if 0, stop
;       cmp al, 10             ; check newline
;       jne skip
;       inc rcx                ; increment line counter
;   skip:
;       inc rsi                ; move to next character
;       jmp loop

section .data
    filename db "names.txt", 0          ; Input file (same directory)
    buffer   times 4096 db 0            ; Buffer for file content
    countMsg db "Total lines: ", 0
    newline  db 10, 0
    errMsg   db "Error: Cannot open file", 10, 0
    errLen   equ $ - errMsg             ; Length of error message

section .bss
    line_count resq 1
    file_len   resq 1
    num_buffer resb 21                  ; buffer for number string

section .text
    global _start

_start:
    ; ===== Open file =====
    mov rax, 2                  ; sys_open
    mov rdi, filename
    mov rsi, 0                  ; O_RDONLY
    syscall

    cmp rax, 0
    js file_error
    mov rdi, rax                ; file descriptor

    ; ===== Read file =====
    mov rax, 0                  ; sys_read
    mov rsi, buffer
    mov rdx, 4096
    syscall
    mov [file_len], rax         ; store bytes read

    ; ===== Close file =====
    mov rax, 3                  ; sys_close
    syscall

    ; ===== Initialize =====
    mov rcx, 0                  ; line counter
    mov rsi, buffer             ; start of buffer
    mov rdx, [file_len]
    add rsi, rdx                ; rsi = buffer + file_len
    mov r8, buffer              ; r8 = current pointer

count_loop:
    cmp r8, rsi
    jge check_last_line
    mov al, [r8]
    cmp al, 10                  ; newline?
    jne not_newline
    inc rcx                     ; increment counter
not_newline:
    inc r8
    jmp count_loop

check_last_line:
    ; If file not empty and last char is not newline, count last line
    cmp qword [file_len], 0
    je print_result
    mov al, [rsi-1]
    cmp al, 10
    je print_result
    inc rcx

print_result:
    mov [line_count], rcx

    ; Print "Total lines: "
    mov rax, 1
    mov rdi, 1
    mov rsi, countMsg
    mov rdx, 13
    syscall

    ; Convert and print number
    mov rax, [line_count]
    call print_number

    ; Print newline
    mov rax, 1
    mov rdi, 1
    mov rsi, newline
    mov rdx, 1
    syscall

    ; Exit
    mov rax, 60
    xor rdi, rdi
    syscall

file_error:
    mov rax, 1
    mov rdi, 1
    mov rsi, errMsg
    mov rdx, errLen
    syscall
    mov rax, 60
    mov rdi, 1
    syscall

; ================================================
; print_number: Converts a number in RAX to ASCII
; ================================================
print_number:
    mov rbx, 10
    lea rsi, [num_buffer + 20]  ; pointer to end of buffer
    mov byte [rsi], 0           ; null-terminate

.convert_loop:
    xor rdx, rdx
    div rbx                     ; divide RAX by 10
    add dl, '0'                 ; remainder -> ASCII
    dec rsi
    mov [rsi], dl
    test rax, rax
    jnz .convert_loop

    ; Print number
    mov rax, 1
    mov rdi, 1
    mov rdx, num_buffer + 20
    sub rdx, rsi                ; length = end - start
    mov rsi, rsi
    syscall
    ret
