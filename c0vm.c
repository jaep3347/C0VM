#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "lib/xalloc.h"
#include "lib/stack.h"
#include "lib/contracts.h"
#include "lib/c0v_stack.h"
#include "lib/c0vm.h"
#include "lib/c0vm_c0ffi.h"
#include "lib/c0vm_abort.h"

/* call stack frames */
typedef struct frame_info frame;
struct frame_info {
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Function body */
  size_t pc;     /* Program counter */
  c0_value *V;   /* The local variables */
};

void free_frames(stack_elem x) {
  free(((frame*)x)->V);
  free(x);
}

int execute(struct bc0_file *bc0) {
  REQUIRES(bc0 != NULL);

  /* Variables */
  c0v_stack_t S = c0v_stack_new(); /* Operand stack of C0 values */
  struct function_info *main_fn = &bc0->function_pool[0];
  ubyte *P = main_fn->code;      /* Array of bytes that make up the current function */
  size_t pc = 0;     /* Current location within the current byte array P */
  c0_value *V = xcalloc(main_fn->num_vars, sizeof(c0_value));;   /* Local variables (you won't need this till Task 2) */
  (void) V;



  /* The call stack, a generic stack that should contain pointers to frames */
  /* You won't need this until you implement functions. */
  gstack_t callStack = stack_new();
  (void) callStack;

  while (true) {

#ifdef DEBUG
    /* You can add extra debugging information here */
    fprintf(stderr, "Opcode %x -- Stack size: %zu -- PC: %zu\n",
            P[pc], c0v_stack_size(S), pc);
#endif

    switch (P[pc]) {

    /* Additional stack operation: */

    case POP: {
      pc++;
      c0v_pop(S);
      break;
    }

    case DUP: {
      pc++;
      c0_value v = c0v_pop(S);
      c0v_push(S,v);
      c0v_push(S,v);
      break;
    }

    case SWAP: {
        pc++;
        c0_value v1 = c0v_pop(S);
        c0_value v2 = c0v_pop(S);
        c0v_push(S, v1);
        c0v_push(S, v2);
        break;
    } 


    /* Returning from a function.
     * This currently has a memory leak! You will need to make a slight
     * change for the initial tasks to avoid leaking memory.  You will
     * need to be revise it further when you write INVOKESTATIC. */

    case RETURN: {
      if (stack_empty(callStack)){
          free(V);
          stack_free(callStack, NULL);
          pc++;
          ASSERT(!c0v_stack_empty(S));
          c0_value result = c0v_pop(S);
          c0v_stack_free(S);
          return (val2int(result));
      } else {
          bool bool_push = false;
          c0_value retval = ptr2val(NULL);
          if (!c0v_stack_empty(S)){
              retval = c0v_pop(S);
              bool_push = true;
          }
          c0v_stack_free(S);
          free(V);
          frame* func = (frame*)pop(callStack);
          ASSERT(func != NULL);
          V = func->V;
          S = func->S;
          P = func->P;
          pc = func->pc;
          free(func);                             
          V = V;
          c0v_push(S,retval);
          if (bool_push == false){
              c0v_pop(S);
          }
        }
      break;
    }


    /* Arithmetic and Logical operations */

    case IADD: {
        pc++;
        uint32_t y = (uint32_t)val2int(c0v_pop(S));
        uint32_t x = (uint32_t)val2int(c0v_pop(S));
        c0v_push(S, int2val((int32_t)(x + y)));
        break;
    }

    case ISUB: {
        pc++;
        uint32_t y = (uint32_t)val2int(c0v_pop(S));
        uint32_t x = (uint32_t)val2int(c0v_pop(S));
        c0v_push(S, int2val((int32_t)(x - y)));
        break;
    }

    case IMUL: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        c0v_push(S, int2val(x * y));
        break;
    }

    case IDIV: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));

        if (y == 0) c0_arith_error("IDIV");
        else if (x == (1 << 31) && y == -1) c0_arith_error("IDIV");
        c0v_push(S, int2val(x / y));
        break;
    }

    case IREM: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        if (y == 0) c0_arith_error("IREM");
        else if (x == (1 << 31) && y == -1) c0_arith_error("IREM");
        c0v_push(S, int2val(x % y));
        break;
    }

    case IAND: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        c0v_push(S, int2val(x & y));
        break;
    }

    case IOR: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        c0v_push(S, int2val(x | y));
        break;
    }

    case IXOR: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        c0v_push(S, int2val(x ^ y));
        break;
    }

    case ISHL: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        if (y > 31 || y<0) c0_arith_error("ISHL");
        c0v_push(S, int2val(x << y));
        break;
    }

    case ISHR: {
        pc++;
        int32_t y = val2int(c0v_pop(S));
        int32_t x = val2int(c0v_pop(S));
        if (y > 31 || y<0) c0_arith_error("ISHR");
        c0v_push(S, int2val(x >> y));
        break;
    }


    /* Pushing constants */

    case BIPUSH: {
        int8_t t = P[pc + 1];
        c0v_push(S, int2val(t));
        pc+=2;
        break;
    }

    case ILDC: {
      pc++;
      uint32_t c1 = (uint32_t)P[pc];
      pc++;
      uint32_t c2 = (uint32_t)P[pc];
      pc++;
      int32_t x = (int)(bc0->int_pool[(c1<<8) | c2]);
      c0v_push(S,int2val(x));
      break;
	}

    case ALDC: {
      pc++;
      uint32_t c1 = (uint32_t)P[pc];
      pc++;
      uint32_t c2 = (uint32_t)P[pc];
      pc++;
	    void* a = &((bc0->string_pool)[(c1<<8) | c2]);
	    c0v_push(S,ptr2val(a));	
	    break;
	}

    case ACONST_NULL: {
        pc++;
        c0v_push(S, int2val(0));
        break;
    }


    /* Operations on local variables */

    case VLOAD: {
        uint8_t i = P[pc+1];
        pc+=2;
        c0v_push(S, V[i]); 
        break;
    }

    case VSTORE: {
        uint8_t i = P[pc+1];
        pc+=2;
        V[i] = c0v_pop(S);
        break;
    }


    /* Assertions and errors */

    case ATHROW: {
      pc++;
      c0_user_error(val2ptr(c0v_pop(S)));
      break;
    }

    case ASSERT: {
      pc++;
      char *a = (char*)val2ptr(c0v_pop(S));
      int32_t x = val2int(c0v_pop(S));
      if (x == 0) {
        c0_assertion_failure(a);
      }
      break;
    }

//////////////////////////////////////////////////////////////////////////////
    /* Control flow operations */

    case NOP: {
      pc++;
      break;
    }

    case IF_CMPEQ: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      c0_value v2 = c0v_pop(S);
      c0_value v1 = c0v_pop(S);
      if (val_equal(v1, v2)) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case IF_CMPNE: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      c0_value v2 = c0v_pop(S);
      c0_value v1 = c0v_pop(S);
      if (!val_equal(v1, v2)) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case IF_ICMPLT: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      int32_t v2 = val2int(c0v_pop(S));
      int32_t v1 = val2int(c0v_pop(S));
      if (v1 < v2) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case IF_ICMPGE: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      int32_t v2 = val2int(c0v_pop(S));
      int32_t v1 = val2int(c0v_pop(S));
      if (v1 >= v2) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case IF_ICMPGT: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      int32_t v2 = val2int(c0v_pop(S));
      int32_t v1 = val2int(c0v_pop(S));
      if (v1 > v2) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case IF_ICMPLE: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      int32_t v2 = val2int(c0v_pop(S));
      int32_t v1 = val2int(c0v_pop(S));
      if (v1 <= v2) {
        pc += (int16_t)((o1 << 8 | o2) - 2);
      } else {
        pc++;
      }
      break;
    }

    case GOTO: {
      pc++;
      int16_t o1 = (int16_t)P[pc];
      pc++;
      int16_t o2 = (int16_t)P[pc];
      pc += (int16_t)((o1 << 8 | o2) - 2);
      break;
    }


    /* Function call operations: */

    case INVOKESTATIC:  {
      pc++;
      uint32_t c1 = (uint32_t)P[pc];
      pc++;
      uint32_t c2 = (uint32_t)P[pc];
      pc++;
      struct function_info fi = bc0->function_pool[c1 << 8 | c2];

      frame *curr = xmalloc(sizeof(frame));
      curr->P = P;
      curr->V = V;
      curr->pc = pc;
      curr->S = S;
      push(callStack, (stack_elem)curr);

      // re initiate all the variables.
      V = xcalloc((uint32_t)fi.num_vars, sizeof(c0_value));

      for (int i = fi.num_args - 1; i >= 0; i--) {
        V[i] = c0v_pop(S);
      }
      S = c0v_stack_new();

      P = fi.code;
      pc = 0;
      break;
    }

    case INVOKENATIVE: {
      pc++;
      uint32_t c1 = (uint32_t)P[pc];
      pc++;
      uint32_t c2 = (uint32_t)P[pc];
      pc++;
      struct native_info nat = bc0->native_pool[(c1<<8) | c2];
      uint16_t num_args = nat.num_args;
      uint16_t index = nat.function_table_index;	
      c0_value* temp_array = xcalloc(num_args,sizeof(c0_value));
      while (num_args != 0){
          temp_array[num_args-1] = c0v_pop(S);
          num_args = num_args - 1;
      }
      native_fn *address = native_function_table[index];
      c0_value result = (*address)(temp_array);
      c0v_push(S,result);
      free(temp_array);
      break;
    }


    /* Memory allocation operations: */

    case NEW: {
      pc++;
      uint8_t c1 = (uint8_t)P[pc];
      void *a = xcalloc(1, c1);
      pc++;
      c0v_push(S, ptr2val(a));
      break;
    }

    case NEWARRAY: {
      pc++;
      uint8_t c1 = (uint8_t)P[pc];
      pc++;
      uint32_t n = (uint32_t)val2int(c0v_pop(S));
      c0_array *array = xmalloc(sizeof(c0_array));
      array->count = n;
      array->elt_size = c1;
      array->elems = xcalloc(n, c1);
      c0v_push(S, ptr2val(array));
      break;
    }

    case ARRAYLENGTH: {
      pc++;
      c0_array *arr = (c0_array*)val2ptr(c0v_pop(S));
      c0v_push(S, int2val(arr->count));
      break;
    }


    /* Memory access operations: */

    case AADDF: {
      pc++;
      uint8_t f = (uint8_t)P[pc];
      pc++;
      void *a = val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to add bytes to NULL pointer");
      }
      c0v_push(S, ptr2val((uint8_t*)a + f));
      break;
    }

    case AADDS: {
      pc++;
      int32_t i = val2int(c0v_pop(S));
      c0_array *a = (c0_array*)val2ptr(c0v_pop(S));
      if (i < 0 || i >= a->count) {
        c0_memory_error("Array access out of bounds.");
      }
      c0v_push(S, ptr2val((uint8_t*)a->elems + (a->elt_size * i)));
      break;
    }

    case IMLOAD: {
      pc++;
      void *a = val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to load NULL pointer");
      }
      c0v_push(S, int2val(*((int32_t*)a)));
      break;
    }

    case IMSTORE: {
      pc++;
      int32_t x = val2int(c0v_pop(S));
      int32_t *a = (int32_t*)val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to store into NULL pointer");
      }
      *a = x;
      break;
    }

    case AMLOAD: {
      pc++;
      void **a = val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to load NULL pointer");
      }
      void *b = *a;
      c0v_push(S, ptr2val(b));
      break;
    }

    case AMSTORE: {
      pc++;
      void *b = val2ptr(c0v_pop(S));
      void **a = val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to store into NULL pointer");
      }
      *a = b;
      break;
    }

    case CMLOAD: {
      pc++;
      void *a = val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to load NULL pointer");
      }
      c0v_push(S, int2val((uint32_t)*((uint8_t*)a)));
      break;
    }

    case CMSTORE: {
      pc++;
      int32_t x = val2int(c0v_pop(S));
      int8_t *a = (int8_t*)val2ptr(c0v_pop(S));
      if (a == NULL) {
        c0_memory_error("Attempt to store into NULL pointer");
      }
      *a = (int8_t)x & 0x7F;
      break;
    }

    default:
      fprintf(stderr, "invalid opcode: 0x%02x\n", P[pc]);
      free(V);
      stack_free(callStack, &free_frames);
      c0v_stack_free(S);
      abort();
    }
  }

  /* cannot get here from infinite loop */
  assert(false);
}
