#pragma once

typedef struct Stack {
  void* data;
  struct Stack* next;
} stack_t;

void stack_push(stack_t** root, void* data);
void* stack_pop(stack_t** root);
void* stack_peek(stack_t* root);
stack_t* create_stack_node(void* data);