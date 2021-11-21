#include "parser/parser.h"
#include "parser/nodes/op_binary.h"

void print_type(FILE* out, const Parser* parser, TypeRef typeref) {
    TypeEntry* entry = typetable_get(&parser->types, typeref);
    Type type = entry->type;
    fprintf(out, "%s (", entry->name);

    switch (type.tag) {
    case TYPE_VOID: fprintf(out, "void"); break;
    case TYPE_INT: fprintf(out, "i%d", (int)type.data); break;
    case TYPE_UINT: fprintf(out, "i%d", (int)type.data); break;
    case TYPE_FLOAT: fprintf(out, "f%d", (int)type.data); break;
    case TYPE_BOOL: fprintf(out, "bool"); break;
    default: fprintf(out, "unknown"); break;
    }

    fprintf(out, ")");
}

void print_item(const Parser* parser, NodeRef noderef, int indent) {
    #define PRINT_TABS(n) for (int i = 0; i < (n); i++) putchar('\t')

    Node* node = arrlist_get(&parser->nodes, noderef);

    PRINT_TABS(indent);
    printf("\033[4m%s\033[0m\n", node->vtable->name);

    TokenRef tokenref = node->vtable->token(parser, node);
    Token* token = arrlist_get(&parser->tokens, tokenref);

    PRINT_TABS(indent+1);
    printf("Token: %.*s (#%d)\n", (int)token->len, token->start, (int)tokenref);

    if (node->vtable->type != NULL) {
        TypeRef type = node->vtable->type(parser, node);
        PRINT_TABS(indent+1);
        printf("Type: ");
        print_type(stdout, parser, type);
        printf("\n");
    }

    if (node->vtable->children != NULL) {
        NodeRefSlice children = node->vtable->children(parser, node);

        PRINT_TABS(indent+1);
        puts("Children:");
        for (int i = 0; i < (int)children.len; i++) {
            putchar('\n');
            print_item(parser, children.data[i], indent+1);
        }
    }
}

void graph_node(FILE* out, const Parser* parser, NodeRef noderef) {
    Node* node = arrlist_get(&parser->nodes, noderef);
    fprintf(out, "%d [shape=\"rectangle\", label=<<B>%s [%d]</B>", (int)noderef, node->vtable->name, (int)noderef);

    TokenRef tokenref = node->vtable->token(parser, node);
    Token* token = arrlist_get(&parser->tokens, tokenref);

    if (token->len != 0 && token->start[0] == '&') {
        fprintf(out, "<BR />Token: &amp; [%d]\n", (int)tokenref);

    } else {
        fprintf(out, "<BR />Token: %.*s [%d]", (int)token->len, token->start, (int)tokenref);
    }

    if (node->vtable->type != NULL) {
        TypeRef type = node->vtable->type(parser, node);
        fprintf(out, "<BR />Type: ");
        print_type(out, parser, type);
    }

    fprintf(out, ">]\n");

    if (node->vtable->children != NULL) {
        NodeRefSlice children = node->vtable->children(parser, node);

        for (int i = 0; i < (int)children.len; i++) {
            fprintf(out, "%d -> %d\n", (int)noderef, (int)children.data[i]);
            graph_node(out, parser, children.data[i]);
        }
    }
}

void graph_create(FILE* out, const Parser* parser, NodeRef root) {
    fprintf(out, "digraph {\n");
    graph_node(out, parser, root);
    fprintf(out, "}\n");
}

int main() {
    Parser parser;
    parser_init(&parser, "1 - 2 ^ 3 & 4 | 5 & 6 | 7 + 9 * 2 == 5");
    NodeRef ref = node_op_binary_parse(&parser);
    printf("%d\n", (int)ref);

    if (parser.error != NULL)
        printf("error: %s\n", parser.error);

//    print_item(&parser, ref, 0);

    FILE* out = fopen("out.txt", "w");
    graph_create(out, &parser, ref);

    return 0;
}