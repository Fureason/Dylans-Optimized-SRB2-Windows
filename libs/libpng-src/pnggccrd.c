 

 

#if (!defined _MSC_VER) && \
    defined(PNG_ASSEMBLER_CODE_SUPPORTED) && \
    defined(PNG_MMX_CODE_SUPPORTED)

int PNGAPI png_dummy_mmx_support(void);

static int _mmx_supported = 2;  

int PNGAPI
png_dummy_mmx_support(void) __attribute__((noinline));

int PNGAPI
png_dummy_mmx_support(void)
{
   int result;
#ifdef PNG_MMX_CODE_SUPPORTED   
    __asm__ __volatile__ (
#ifdef __x86_64__
        "pushq %%rbx          \n\t"   
        "pushq %%rcx          \n\t"   
        "pushq %%rdx          \n\t"   
        "pushfq               \n\t"   
        "popq %%rax           \n\t"   
        "movq %%rax, %%rcx    \n\t"   
        "xorl $0x200000, %%eax \n\t"  
        "pushq %%rax          \n\t"   
        "popfq                \n\t"   
        "pushfq               \n\t"   
        "popq %%rax           \n\t"   
        "pushq %%rcx          \n\t"   
        "popfq                \n\t"   
#else
        "pushl %%ebx          \n\t"   
        "pushl %%ecx          \n\t"   
        "pushl %%edx          \n\t"   
        "pushfl               \n\t"   
        "popl %%eax           \n\t"   
        "movl %%eax, %%ecx    \n\t"   
        "xorl $0x200000, %%eax \n\t"  
        "pushl %%eax          \n\t"   
        "popfl                \n\t"   
        "pushfl               \n\t"   
        "popl %%eax           \n\t"   
        "pushl %%ecx          \n\t"   
        "popfl                \n\t"   
#endif
        "xorl %%ecx, %%eax    \n\t"   
        "jz 0f                \n\t"   

        "xorl %%eax, %%eax    \n\t"   

        "cpuid                \n\t"   
        "cmpl $1, %%eax       \n\t"   
        "jl 0f                \n\t"   

        "xorl %%eax, %%eax    \n\t"   
        "incl %%eax           \n\t"   

        "cpuid                \n\t"   
        "andl $0x800000, %%edx \n\t"  
        "cmpl $0, %%edx       \n\t"   
        "jz 0f                \n\t"   

        "movl $1, %%eax       \n\t"   
        "jmp  1f              \n\t"   

    "0:                       \n\t"   
        "movl $0, %%eax       \n\t"   
    "1:                       \n\t"   
#ifdef __x86_64__
        "popq %%rdx           \n\t"   
        "popq %%rcx           \n\t"   
        "popq %%rbx           \n\t"   
#else
        "popl %%edx           \n\t"   
        "popl %%ecx           \n\t"   
        "popl %%ebx           \n\t"   
#endif




        : "=a" (result)               

        :                             





    );
    _mmx_supported = result;
#else
    _mmx_supported = 0;
#endif  

    return _mmx_supported;
}
#endif
