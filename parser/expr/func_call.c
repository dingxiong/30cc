#include "../../libc.h"
#include "../../lexer.h"
#include "../parser.h"
#include "func_call.h"
#include "../func.h"
#include "expr.h"

void func_call_debug(int depth, parser_node *node)
{
    node_func_call *call = (node_func_call *)node->data;
    printtabs(depth);
    printf("FunctionCall:\n");
    printtabs(depth + 1);
    printf("Function:\n");
    call->func->debug(depth + 2, call->func);
    printtabs(depth + 1);
    printf("Args:\n");
    for (int i = 0; i < call->num_args; i++)
    {
        call->args[i]->debug(depth + 2, call->args[i]);
    }
}

apply_result *func_call_apply(parser_node *node, context *ctx)
{
    node_func_call *call = (node_func_call *)node->data;

    char **argvals = (char **)malloc(sizeof(char *) * 6);
    general_type **argtypes = (general_type **)malloc(sizeof(general_type *) * 6);
    for (int i = 0; i < call->num_args; i++)
    {
        apply_result *regval = call->args[i]->apply(call->args[i], ctx);
        if (regval->type->kind == TYPE_STRUCT)
        {
            fprintf(stderr, "Passing structs to functions is unsupported!\n");
            exit(1);
        }

        symbol *tmp = new_temp_symbol(ctx, regval->type);
        char *rega = reg_a(regval->type, ctx);
        int is_str_literal = 0;
        if (regval->type->kind == TYPE_POINTER)
        {
            general_type* base = ((pointer_type*)regval->type->data)->of;
            if (base->kind == TYPE_PRIMITIVE && strcmp(((primitive_type*)base->data)->type_name, "TKN_CHAR") == 0) 
            {
              is_str_literal = 1;
            }
        } 
        if (is_str_literal)
        {

            add_text(ctx, "adrp %s, %s@PAGE", rega, regval->code);
            add_text(ctx, "add %s, %s, %s@PAGEOFF", rega, rega, regval->code);
        }
        else 
        {
            add_text(ctx, "mov %s, %s", rega, regval->code);
        }
        add_text(ctx, "str %s, %s", rega, tmp->repl);

        argvals[i] = tmp->repl;
        argtypes[i] = regval->type;
    }
    for (int i = 0; i < call->num_args; i++)
    {
        char *regname = NULL;
        if (i == 0)
            regname = "x0";
        else if (i == 1)
            regname = "x1";
        else if (i == 2)
            regname = "x2";
        else if (i == 3)
            regname = "x3";
        else if (i == 4)
            regname = "x4";
        else if (i == 5)
            regname = "x5";
        else
        {
            fprintf(stderr, "Cannot provide more than 6 args!\n");
            exit(1);
        }
        regname = reg_typed(regname, argtypes[i], ctx);
        add_text(ctx, "ldr %s, %s", regname, argvals[i]);
    }

    apply_result *fun_obj = call->func->apply(call->func, ctx);
    if (fun_obj->type->kind != TYPE_POINTER)
    {
        fprintf(stderr, "Expected a pointer to function!");
        exit(1);
    }
    general_type *fun_type = ((pointer_type *)fun_obj->type->data)->of;

    if (fun_type->kind != TYPE_FUNC)
    {
        fprintf(stderr, "Expected a pointer to function!");
        exit(1);
    }
    general_type *ret_type = ((func_type *)fun_type->data)->return_type;

    // MacOS has different ABI for variadic function
    // TODO: improve this lookup
    int is_variadic = 0;
    int num_fix_params = 0;
    list_node *curr = ctx->functions->first;
    while (curr)
    {
        parser_node* node = (parser_node*)curr->value;
        node_func_def* func_def = (node_func_def*)node->data;
        if (strcmp(func_def->identity, fun_obj->code) == 0)
        {
            is_variadic = func_def->is_variadic;
            num_fix_params = func_def->num_params;
            break;
        }
        curr = curr->next;
    }
    
    if (is_variadic) 
    {
        // variadic arguments should also be put inside stack
        for (int i = num_fix_params; i < call->num_args; i++) 
        {
            add_text(ctx, "str x%d, [sp, #-16]!", i);
        }
    }
    add_text(ctx, "bl %s", fun_obj->code);
    if (is_variadic) 
    {
        // restore sp
        for (int i = num_fix_params; i < call->num_args; i++) 
        {
            add_text(ctx, "add sp, sp, #16");
        }
    }

    char *rega = reg_a(ret_type, ctx);
    // If return type is not void
    if (rega)
    {
        symbol *tmp = new_temp_symbol(ctx, ret_type);
        add_text(ctx, "str %s, %s", rega, tmp->repl);
        return new_result(tmp->repl, tmp->type);
    }
    else
    {
        return NULL;
    }
}

parser_node *parse_func_call(typed_token **tkns_ptr, parser_node *func)
{
    typed_token *tkn = *tkns_ptr;

    if (tkn->type_id == TKN_L_PAREN)
    {
        tkn = tkn->next;
        int num_args = 0;
        parser_node **args = (parser_node **)malloc(sizeof(parser_node *) * 32);
        while (tkn)
        {
            if (tkn->type_id == TKN_R_PAREN)
            {
                tkn = tkn->next;
                *tkns_ptr = tkn;

                parser_node *node = new_node(func_call_debug, func_call_apply, sizeof(node_func_call));
                node_func_call *call = (node_func_call *)node->data;

                call->func = func;
                call->num_args = num_args;
                call->args = args;

                return node;
            }
            parser_node *arg = parse_expr(&tkn);
            if (arg)
            {
                args[num_args++] = arg;
            }
            else
            {
                return NULL;
            }

            if (tkn->type_id == TKN_COMMA)
            {
                tkn = tkn->next;
            }
            // TODO: I believe below else branch is redundant.
            else
            {
                if (tkn->type_id != TKN_R_PAREN)
                {
                    return NULL;
                }
            }
        }
    }

    return NULL;
}
