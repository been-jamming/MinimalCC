.data
__str0:
.asciiz ","

__str1:
.asciiz "\n"

.text

.globl entry
entry:
addi $sp, $sp, -116
addi $s0, $sp, 8
li $s1, 0
sw $s1, 0($s0)
move $s0, $s1
addi $s0, $sp, 4
li $s1, 0
sw $s1, 0($s0)
move $s0, $s1

__L0:
lw $s0, 4($sp)
li $s1, 10
slt $s0, $s0, $s1
beq $s0, $zero, __L1

__L2:
lw $s0, 8($sp)
li $s1, 10
slt $s0, $s0, $s1
beq $s0, $zero, __L3
addi $s0, $sp, 9
lw $s1, 8($sp)
li $t0, 10
mult $s1, $t0
mflo $s1
add $s0, $s0, $s1
lw $s1, 4($sp)
add $s0, $s0, $s1
lw $s1, 8($sp)
li $s2, 10
lw $s3, 4($sp)
mult $s2, $s3
mflo $s2
add $s1, $s1, $s2
sll $s1, $s1, 24
sra $s1, $s1, 24
sb $s1, 0($s0)
move $s0, $s1
la $s0, printd
sw $s0, 0($sp)
la $t0, __L4
sw $t0, -4($sp)
addi $t0, $sp, 9
sw $t0, -12($sp)
lw $s0, 8($sp)
li $t0, 10
mult $s0, $t0
mflo $s0
lw $t0, -12($sp)
add $t0, $t0, $s0
sw $t0, -12($sp)
lw $s0, 4($sp)
lw $t0, -12($sp)
add $t0, $t0, $s0
lb $t0, 0($t0)
sw $t0, -12($sp)
lb $t0, -12($sp)
sw $t0, -12($sp)
lw $t0, 0($sp)
addi $sp, $sp, -4
jr $t0

__L4:
addi $sp, $sp, 4
lw $t0, -8($sp)
lw $s0, 0($sp)
move $s0, $t0
lw $s0, 8($sp)
li $s1, 9
sne $s0, $s0, $s1
beq $s0, $zero, __L5
la $s0, prints
sw $s0, 0($sp)
la $t0, __L6
sw $t0, -4($sp)
la $t0, __str0
sw $t0, -12($sp)
lw $t0, 0($sp)
addi $sp, $sp, -4
jr $t0

__L6:
addi $sp, $sp, 4
lw $t0, -8($sp)
lw $s0, 0($sp)
move $s0, $t0

__L5:
addi $s0, $sp, 8
lw $s1, 8($sp)
li $s2, 1
add $s1, $s1, $s2
sw $s1, 0($s0)
move $s0, $s1
j __L2

__L3:
la $s0, prints
sw $s0, 0($sp)
la $t0, __L7
sw $t0, -4($sp)
la $t0, __str1
sw $t0, -12($sp)
lw $t0, 0($sp)
addi $sp, $sp, -4
jr $t0

__L7:
addi $sp, $sp, 4
lw $t0, -8($sp)
lw $s0, 0($sp)
move $s0, $t0
addi $s0, $sp, 4
lw $s1, 4($sp)
li $s2, 1
add $s1, $s1, $s2
sw $s1, 0($s0)
move $s0, $s1
addi $s0, $sp, 8
li $s1, 0
sw $s1, 0($s0)
move $s0, $s1
j __L0

__L1:
addi $sp, $sp, 116
lw $ra, 0($sp)
jr $ra
