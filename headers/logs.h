#include <stdio.h>


#if !defined(LOGS_H)
#define LOGS_H


#if defined(JSON_DO_LOGS)
    #define LOGS_SCOPE(name)\
        const char* logs__scope__name = #name;\
        if(!logs__scope__name[0]) logs__scope__name = __func__;\
        printf("LOGS '%s'\n", logs__scope__name);\

    #define LOG(...)\
        do {\
            printf("LOGS '%s' ", logs__scope__name);\
            printf(__VA_ARGS__);\
        } while(0);\
    
    #define LOG_LN(...)\
        do {\
            LOG(__VA_ARGS__);\
            printf("\n");\
        } while(0);\

    #define CHECK(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, LOCAL_STATUS);\
                printf(__VA_ARGS__);\
                printf("\n");\
                return LOCAL_STATUS;\
            }\
        } while(0);\

    
    #define CHECK_BOOLEAN(condition, status, ...)\
        do {\
            if(!(condition)) {\
                printf("LOGS '%s' Failed bool check ", logs__scope__name);\
                printf(__VA_ARGS__);\
                printf("\n");\
                return (status);\
            }\
        } while(0);\

    #define LOGS_ONLY(code) code

    #define CHECK_ELSE(condition, elseCode, ...)\
        do {\
            int CONDITION_VALUE = (condition);\
            if(CONDITION_VALUE < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, CONDITION_VALUE);\
                printf(__VA_ARGS__);\
                printf("\n");\
                elseCode;\
            }\
        } while(0);
    
    #define CHECK_RETURN(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, LOCAL_STATUS);\
                printf(__VA_ARGS__);\
                printf("\n");\
            }\
            return LOCAL_STATUS;\
        } while(0);\

#else
    #define LOGS_SCOPE(name)
    #define LOG(...)
    #define LOG_LN(...)

    #define CHECK(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) return LOCAL_STATUS;\
        } while(0);\

    
    #define CHECK_BOOLEAN(condition, status, ...)\
        do {\
            if(!(condition)) return (status);\
        } while(0);\

    #define LOGS_ONLY(code)

    #define CHECK_ELSE(condition, elseCode, ...)\
        do {\
            int CONDITION_VALUE = (condition);\
            if(CONDITION_VALUE < 0) {\
                elseCode;\
            }\
        } while(0);
    
    #define CHECK_RETURN(condition, ...) return (condition);

#endif // DO_LOGS


#endif // LOGS_H
