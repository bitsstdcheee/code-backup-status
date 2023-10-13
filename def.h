#ifdef Compile_Markdown
    #ifdef Compile_Plain
        #error "Compile_Markdown & Compile_Plain cannot be defined at one time."
    #else
        #ifndef OUT_Markdown
            #define OUT_Markdown 
        #endif
        
        #ifndef OUT_ColorAC
            #define OUT_ColorAC 
        #endif
        
        #ifndef OUT_DoubleBackslash
            #define OUT_DoubleBackslash 
        #endif
        
        #ifndef OUT_ProblemUrl
            #define OUT_ProblemUrl 
        #endif

        #ifndef OUT_Badge
            #define OUT_Badge
        #endif
    #endif
#else
    #ifdef Compile_Plain
        // Nothing to define
    #endif
#endif