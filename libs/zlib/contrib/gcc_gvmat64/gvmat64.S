/*
;uInt longest_match_x64(
;    deflate_state *s,
;    IPos cur_match);                             // current match 

; gvmat64.S -- Asm portion of the optimized longest_match for 32 bits x86_64
;  (AMD64 on Athlon 64, Opteron, Phenom
;     and Intel EM64T on Pentium 4 with EM64T, Pentium D, Core 2 Duo, Core I5/I7)
; this file is translation from gvmat64.asm to GCC 4.x (for Linux, Mac XCode)
; Copyright (C) 1995-2010 Jean-loup Gailly, Brian Raiter and Gilles Vollant.
;
; File written by Gilles Vollant, by converting to assembly the longest_match
;  from Jean-loup Gailly in deflate.c of zLib and infoZip zip.
;  and by taking inspiration on asm686 with masm, optimised assembly code
;        from Brian Raiter, written 1998
;
;  This software is provided 'as-is', without any express or implied
;  warranty.  In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Permission is granted to anyone to use this software for any purpose,
;  including commercial applications, and to alter it and redistribute it
;  freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not
;     claim that you wrote the original software. If you use this software
;     in a product, an acknowledgment in the product documentation would be
;     appreciated but is not required.
;  2. Altered source versions must be plainly marked as such, and must not be
;     misrepresented as being the original software
;  3. This notice may not be removed or altered from any source distribution.
;
;         http://www.zlib.net
;         http://www.winimage.com/zLibDll
;         http://www.muppetlabs.com/~breadbox/software/assembly.html
;
; to compile this file for zLib, I use option:
;   gcc -c -arch x86_64 gvmat64.S


;uInt longest_match(s, cur_match)
;    deflate_state *s;
;    IPos cur_match;                             // current match /
;
; with XCode for Mac, I had strange error with some jump on intel syntax
; this is why BEFORE_JMP and AFTER_JMP are used
 */


#define BEFORE_JMP .att_syntax
#define AFTER_JMP .intel_syntax noprefix

#ifndef NO_UNDERLINE
#	define	match_init	_match_init
#	define	longest_match	_longest_match
#endif

.intel_syntax noprefix

.globl	match_init, longest_match
.text
longest_match:



#define LocalVarsSize 96
  
                                                           
                          
                             
  

#define chainlenwmask     (rsp + 8 - LocalVarsSize)
#define nicematch         (rsp + 16 - LocalVarsSize)

#define save_rdi        (rsp + 24 - LocalVarsSize)
#define save_rsi        (rsp + 32 - LocalVarsSize)
#define save_rbx        (rsp + 40 - LocalVarsSize)
#define save_rbp        (rsp + 48 - LocalVarsSize)
#define save_r12        (rsp + 56 - LocalVarsSize)
#define save_r13        (rsp + 64 - LocalVarsSize)
#define save_r14        (rsp + 72 - LocalVarsSize)
#define save_r15        (rsp + 80 - LocalVarsSize)


  
                                                                          
                                                                      
                                                            
                                                                     
  

#define    MAX_MATCH              258
#define    MIN_MATCH              3
#define    MIN_LOOKAHEAD          (MAX_MATCH+MIN_MATCH+1)

  
                                                                    
                                                                 
                                                                    
                                                                    
                                                              
                        

                                                                 
                                                                              
                                                                         
                                                                                 
                                                                        
  



                                                

                   
                  
                    

                 
 
                 
                     
                                               

                                                                               
                                                                               
                                                                               
                                                                             
                                                                                     
                                                                                   
                                                                                 
                                                                                    
                                                                                  
                                                                                    
                                                                                         
                                                                                   
                                                                                   
 
  

#define dsWSize          68
#define dsWMask          76
#define dsWindow         80
#define dsPrev           96
#define dsMatchLen       144
#define dsPrevMatch      148
#define dsStrStart       156
#define dsMatchStart     160
#define dsLookahead      164
#define dsPrevLen        168
#define dsMaxChainLen    172
#define dsGoodMatch      188
#define dsNiceMatch      192

#define window_size      [ rcx + dsWSize]
#define WMask            [ rcx + dsWMask]
#define window_ad        [ rcx + dsWindow]
#define prev_ad          [ rcx + dsPrev]
#define strstart         [ rcx + dsStrStart]
#define match_start      [ rcx + dsMatchStart]
#define Lookahead        [ rcx + dsLookahead]  
#define prev_length      [ rcx + dsPrevLen]
#define max_chain_length [ rcx + dsMaxChainLen]
#define good_match       [ rcx + dsGoodMatch]
#define nice_match       [ rcx + dsNiceMatch]

  
          
                                                                 

                                                                          
                                                                                                                
 
                                                             
                                                           

 
                      
                                                      
                                
                                             

                                                                    
                                  


                                                            
                                                                    
                                                               
                           

                                                                             
                                  
                                                                        
  
        mov [save_rbx],rbx
        mov [save_rbp],rbp


        mov rcx,rdi

        mov r8d,esi


        mov [save_r12],r12
        mov [save_r13],r13
        mov [save_r14],r14
        mov [save_r15],r15









        mov edi, prev_length
        mov esi, good_match
        mov eax, WMask
        mov ebx, max_chain_length
        cmp edi, esi
        jl  LastMatchGood
        shr ebx, 2
LastMatchGood:






        dec ebx
        shl ebx, 16
        or  ebx, eax






        mov eax, nice_match
        mov [chainlenwmask], ebx
        mov r10d, Lookahead
        cmp r10d, eax
        cmovnl r10d, eax
        mov [nicematch],r10d




        mov r10, window_ad
        mov ebp, strstart
        lea r13, [r10 + rbp]




         mov r9,r13
         neg r13
         and r13,3





        mov eax, window_size
        sub eax, MIN_LOOKAHEAD


        xor edi,edi
        sub ebp, eax

        mov r11d, prev_length

        cmovng ebp,edi






       lea  rsi,[r10+r11]





        movzx r12d,word ptr [r9]
        movzx ebx, word ptr [r9 + r11 - 1]

        mov rdi, prev_ad



        mov edx, [chainlenwmask]

        cmp bx,word ptr [rsi + r8 - 1]
        jz  LookupLoopIsZero
				
						
						
LookupLoop1:
        and r8d, edx

        movzx   r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
        jbe LeaveNow
		
		
		
        sub edx, 0x00010000
		BEFORE_JMP
        js  LeaveNow
		AFTER_JMP

LoopEntry1:
        cmp bx,word ptr [rsi + r8 - 1]
		BEFORE_JMP
        jz  LookupLoopIsZero
		AFTER_JMP

LookupLoop2:
        and r8d, edx

        movzx   r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
		BEFORE_JMP
        jbe LeaveNow
		AFTER_JMP
        sub edx, 0x00010000
		BEFORE_JMP
        js  LeaveNow
		AFTER_JMP

LoopEntry2:
        cmp bx,word ptr [rsi + r8 - 1]
		BEFORE_JMP
        jz  LookupLoopIsZero
		AFTER_JMP

LookupLoop4:
        and r8d, edx

        movzx   r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
		BEFORE_JMP
        jbe LeaveNow
		AFTER_JMP
        sub edx, 0x00010000
		BEFORE_JMP
        js  LeaveNow
		AFTER_JMP

LoopEntry4:

        cmp bx,word ptr [rsi + r8 - 1]
		BEFORE_JMP
        jnz LookupLoop1
        jmp LookupLoopIsZero
		AFTER_JMP
  
        
                                      
                                                     
                                                  
             
                                                          
                                     
   
                                                                       
                                                                     
                                           
   
                     
                 
                  
                                                          
                                                  
              
               
  
.balign 16
LookupLoop:
        and r8d, edx

        movzx   r8d, word ptr [rdi + r8*2]
        cmp r8d, ebp
		BEFORE_JMP
        jbe LeaveNow
		AFTER_JMP
        sub edx, 0x00010000
		BEFORE_JMP
        js  LeaveNow
		AFTER_JMP

LoopEntry:

        cmp bx,word ptr [rsi + r8 - 1]
		BEFORE_JMP
        jnz LookupLoop1
		AFTER_JMP
LookupLoopIsZero:
        cmp     r12w, word ptr [r10 + r8]
		BEFORE_JMP
        jnz LookupLoop1
		AFTER_JMP



        mov [chainlenwmask], edx
  
                                                                    
                                                                 
                                                                  
                                              
  
        lea rsi,[r8+r10]
        mov rdx, 0xfffffffffffffef8  
        lea rsi, [rsi + r13 + 0x0108]  
        lea rdi, [r9 + r13 + 0x0108]  

        prefetcht1 [rsi+rdx]
        prefetcht1 [rdi+rdx]

  
                                                                 
                                                                      
   
                                                                   
                                                                       
                                                                      
                                                               
                                                                    
   
                                                                    
                                                            
                                                             
                
  

LoopCmps:
        mov rax, [rsi + rdx]
        xor rax, [rdi + rdx]
        jnz LeaveLoopCmps

        mov rax, [rsi + rdx + 8]
        xor rax, [rdi + rdx + 8]
        jnz LeaveLoopCmps8


        mov rax, [rsi + rdx + 8+8]
        xor rax, [rdi + rdx + 8+8]
        jnz LeaveLoopCmps16

        add rdx,8+8+8

		BEFORE_JMP
        jnz  LoopCmps
        jmp  LenMaximum
		AFTER_JMP
		
LeaveLoopCmps16: add rdx,8
LeaveLoopCmps8: add rdx,8
LeaveLoopCmps:

        test    eax, 0x0000FFFF
        jnz LenLower

        test eax,0xffffffff

        jnz LenLower32

        add rdx,4
        shr rax,32
        or ax,ax
		BEFORE_JMP
        jnz LenLower
		AFTER_JMP

LenLower32:
        shr eax,16
        add rdx,2
		
LenLower:		
        sub al, 1
        adc rdx, 0



        lea rax, [rdi + rdx]
        sub rax, r9
        cmp eax, MAX_MATCH
		BEFORE_JMP
        jge LenMaximum
		AFTER_JMP
  
                                                                   
                                                              
                                    
  
        cmp eax, r11d
        jg  LongerMatch

        lea rsi,[r10+r11]

        mov rdi, prev_ad
        mov edx, [chainlenwmask]
		BEFORE_JMP
        jmp LookupLoop
		AFTER_JMP
  
                                       
                           
                                         
                                                 
  
LongerMatch:
        mov r11d, eax
        mov match_start, r8d
        cmp eax, [nicematch]
		BEFORE_JMP
        jge LeaveNow
		AFTER_JMP

        lea rsi,[r10+rax]

        movzx   ebx, word ptr [r9 + rax - 1]
        mov rdi, prev_ad
        mov edx, [chainlenwmask]
		BEFORE_JMP
        jmp LookupLoop
		AFTER_JMP



LenMaximum:
        mov r11d,MAX_MATCH
        mov match_start, r8d




LeaveNow:
        mov eax, Lookahead
        cmp r11d, eax
        cmovng eax, r11d








        mov rbx,[save_rbx]
        mov rbp,[save_rbp]
        mov r12,[save_r12]
        mov r13,[save_r13]
        mov r14,[save_r14]
        mov r15,[save_r15]


        ret 0






match_init:
  ret 0


