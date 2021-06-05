#include <stddef.h>
#include <setjmp.h>

#define E4C_MAX_FRAMES 16

#define E4C_MESSAGE_SIZE 128

#define E4C_DEBUG_INFO __FILE__, __LINE__

struct e4c_exception_type{
    const char * name;
    const char * default_message;
    const struct e4c_exception_type * supertype;
};

struct e4c_exception_type RuntimeException = { "RuntimeException", "Runtime exception.", &RuntimeException };
struct e4c_exception_type NullPointerException = { "NullPointerException", "Null pointer.", &RuntimeException };
struct e4c_exception_type ColorException = { "ColorException", "Color exception.", &RuntimeException };
struct e4c_exception_type RedException = { "RedException", "Red exception.", &ColorException };

struct e4c_exception{
    char message[E4C_MESSAGE_SIZE];
    const char * file;
    int line;
    const struct e4c_exception_type * type;
};

extern struct e4c_context
{
	jmp_buf jump[E4C_MAX_FRAMES]; 
	struct e4c_exception err; 
	struct{unsigned char stage; unsigned char uncaught;} frame[E4C_MAX_FRAMES + 1]; 
	int frames;
} e4c;

struct e4c_context e4c = {0};
static const char * err_msg[] = {"\n\nError: %s (%s)\n\n", "\n\nUncaught %s: %s\n\n    thrown at %s:%d\n\n"};

#define E4C_IS_INSTANCE_OF(t) ( e4c.err.type == &t || e4c_extends(e4c.err.type, &t) )

int e4c_extends(const struct e4c_exception_type * child, const struct e4c_exception_type * parent)
{
    	while ((child) && (child->supertype != child))
	{
        	if(child->supertype == parent)
		{
            		return 1;
        	}

		child = child->supertype;
    	}

    	return 0;
}

#define E4C_THROW(type, message) e4c_throw(&type, E4C_DEBUG_INFO, message)

void e4c_throw(const struct e4c_exception_type * exception_type, const char * file, int line, const char * message)
{
    	e4c.err.type = exception_type;
    	e4c.err.file = file;
    	e4c.err.line = line;

	printf("Exception: %s\n", exception_type->name);

    	e4c.frame[e4c.frames].uncaught = 1;

    	if(e4c.frames > 0)
	{
        	longjmp(e4c.jump[e4c.frames - 1], 1);
    	}

	exit(1);
}

enum e4c_stage{e4c_beginning, e4c_trying, e4c_catching, e4c_finalizing, e4c_done};

#define E4C_TRY if(e4c_try(E4C_DEBUG_INFO) && setjmp(e4c.jump[e4c.frames - 1]) >= 0) while(e4c_hook_try()) if(e4c.frame[e4c.frames].stage == e4c_trying)
#define E4C_CATCH(type) else if(e4c.frame[e4c.frames].stage == e4c_catching && E4C_IS_INSTANCE_OF(type) && e4c_hook_catch())

int e4c_try(const char * file, int line)
{
	if(e4c.frames >= E4C_MAX_FRAMES)
	{
        	e4c_throw(&RuntimeException, file, line, "Too many `try` blocks nested.");
    	}

    	e4c.frames++;

    	e4c.frame[e4c.frames].stage = e4c_beginning;
    	e4c.frame[e4c.frames].uncaught = 0;

    	return 1;
}

int e4c_hook_try()
{
	int uncaught;

    	uncaught = e4c.frame[e4c.frames].uncaught;

    	e4c.frame[e4c.frames].stage++;
    	if(e4c.frame[e4c.frames].stage == e4c_catching && !uncaught)
	{
        	e4c.frame[e4c.frames].stage++;
    	}

    	if(e4c.frame[e4c.frames].stage < e4c_done)
	{
        	return 1;
    	}
	else
	{
    		e4c.frames--;

    		if(uncaught)
		{
        		e4c.frame[e4c.frames].uncaught = 1;

    			if(e4c.frames > 0)
			{
        			longjmp(e4c.jump[e4c.frames - 1], 1);
    			}

			exit(1);
    		}

    		return 0;
	}
}

int e4c_hook_catch()
{
        e4c.frame[e4c.frames].uncaught = 0;
        return 1;
}

void nest_try_block(int keep_nesting){

    if(keep_nesting){

        E4C_TRY{

            nest_try_block(--keep_nesting);
        }
    }
}

int main()
{
	//E4C_THROW(NullPointerException, "This is an uncaught exception");
	//nest_try_block(E4C_MAX_FRAMES /* will overflow */);

	E4C_TRY{

        	E4C_THROW(RedException, "This is a red exception");

    	}E4C_CATCH(ColorException){

        	printf("The color exception was caught: %s\n", e4c.err.type->name);

    	}

	return 0;
}
