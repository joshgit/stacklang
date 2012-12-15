#include <unistd.h> 
#include <ctype.h> 
#include <stdio.h> 
#include <string.h> 
#include <time.h> 

// modes
#define NORMAL 0
#define READ_DEF 1

#define NO_IF 0
#define NOT_SKIPPING 1
#define SKIPPING 2

int mode = NORMAL;
int if_mode = NO_IF;

// variables for stack
int stack[1024];
int stack_pos = -1;

// variables for getting input
FILE *current_input;
char buf[BUFSIZ];
const int WORD_SIZE = 256;
char current_word[256];
char current_string[256];
char* buf_pos = NULL;

const char const *START_STRING = "s\"";

struct word_list {
    char *word;
    struct word_list *next;
};

struct def_list {
    struct word_list *def;
    struct def_list *next;
};

struct word_list *current_list = NULL;
struct word_list *end_current_list = NULL;

struct def_list *defs_list = NULL;
struct def_list *end_defs_list = NULL;

void get_next_word();
void read_string();
void append_word_to_current_def(const char *string);
void append_current_def();
void print_defs();
void eval_word(const char *word);
struct word_list *find_def(const char *word);
void eval_def(struct word_list *user_def);

int main()
{
    *buf = (char) NULL;
    buf_pos = buf;

    get_next_word ();
    while (*current_word != (char) NULL) {

        printf ("current_word %s\n", current_word);

        if (!strcmp(current_word, ":")) { // starting to read def

            if (READ_DEF == mode) {
                printf ("Already reading a def.\n");
            }
            else {
                current_list = NULL;
                end_current_list = NULL;
                mode = READ_DEF;
            }

        }
        else if (!strcmp(current_word, ";")) { // ending reading of def

            if (READ_DEF != mode) {
                printf ("Not reading a def.\n");
            }
            else {
                append_current_def ();
                print_defs ();
                mode = NORMAL;
            }

        }
        else if (!strcmp(current_word, START_STRING)) {

            read_string ();
            printf ("read string: %s\n", current_string);

            if (NORMAL == mode) { // push string address
                stack_pos++;
                stack[stack_pos] = current_string;
            }
            else if (READ_DEF == mode) {
                append_word_to_current_def (START_STRING);
                append_word_to_current_def (current_string);
            }

        }
        else if (READ_DEF == mode) {

            printf ("read_def mode: %s\n", current_word);
            append_word_to_current_def (current_word);

        }
        else if (NORMAL == mode) { // for now only print word in normal mode

            printf ("normal mode: %s\n", current_word);
            eval_word (current_word);
        }

        get_next_word ();
    }

    return 0;
}


struct word_list *find_def(const char *word) {

    struct def_list *temp;

    for (temp = defs_list; NULL != temp; temp = temp->next) {
        if (!strcmp(temp->def->word, word)) {
            return temp->def;
        }
    }
    return NULL;

}


void eval_def(struct word_list *user_def) {
    struct word_list *temp = user_def;
    struct word_list *user_word;
    clock_t  start = clock ();

    printf ("Eval def: ");
    while (NULL != temp) {
        printf ("%s ", temp->word);
        temp = temp->next;
    }
    printf ("\n");

    temp = user_def;
    if (temp != NULL) {
        temp = temp->next;
    }
    while (NULL != temp) {

        user_word = find_def(temp->word);
        if (NULL != user_word)  printf ("Evaling user word %s.\n", user_word->word);
        else                    printf ("Evaling word %s.\n", temp->word);

        eval_word (temp->word);

        if (NULL != user_word) printf ("Done evaling user word, next %s.\n", temp->next->word);
        temp = temp->next;
    }
    printf ("\n");
    printf ("Done evaling %s, %d ms passed.\n", user_def->word, clock () - start);
}


void eval_word(const char *word) {

    int temp, old_if_mode;
    struct word_list *user_def;

    printf ("eval_word got %s\n", word);

    if (!strcmp(word, "IF")) {
        stack_pos--;
        if (stack[stack_pos + 1] > 0) {
            if_mode = NOT_SKIPPING;
        }
        else {
            if_mode = SKIPPING;
        }
    }
    else if (!strcmp(word, "ELSE")) {
        if (NOT_SKIPPING == if_mode) {
            if_mode = SKIPPING;
        }
        else {
            if_mode = NOT_SKIPPING;
        }
    }
    else if (!strcmp(word, "ENDIF")) {
        if_mode = NO_IF;
    }
    else if (SKIPPING == if_mode) {
        printf ("skipping %s\n", word);
    }
    else if (!strcmp(word, "<")) {
        stack[stack_pos - 1] = (stack[stack_pos] < stack[stack_pos - 1]) ? 1 : 0;
        stack_pos--;
    }
    else if (!strcmp(word, "<=")) {
        stack[stack_pos - 1] = (stack[stack_pos] <= stack[stack_pos - 1]) ? 1 : 0;
        stack_pos--;
    }
    else if (!strcmp(word, "==")) {
        stack[stack_pos - 1] = (stack[stack_pos - 1] == stack[stack_pos]) ? 1 : 0;
        stack_pos--;
    }
    else if (!strcmp(word, "!=")) {
        stack[stack_pos - 1] = (stack[stack_pos - 1] == stack[stack_pos]) ? 0 : 1;
        stack_pos--;
    }
    else if (!strcmp(word, "pop")) {
        printf ("pop: %d\n", stack[stack_pos]);
        stack_pos--;
    }
    else if (!strcmp(word, "over")) {
        stack_pos++;
        stack[stack_pos] = stack[stack_pos - 2];
    }
    else if (!strcmp(word, "swap")) {
        temp = stack[stack_pos];
        stack[stack_pos] = stack[stack_pos - 1];
        stack[stack_pos - 1] = temp;
    }
    else if (!strcmp(word, ".")) {
        for (temp = 0; temp <= stack_pos; temp++) {
            printf ("%d ", stack[temp]);
        }
        printf ("\n");
    }
    else if (!strcmp(word, "s.")) {
        printf ("%s\n", (char *)stack[stack_pos]);
    }
    else if (!strcmp(word, "dup")) {
        stack_pos++;
        stack[stack_pos] = stack[stack_pos - 1];
    }
    else if (!strcmp(word, "2dup")) {
        stack_pos += 2;
        stack[stack_pos] = stack[stack_pos - 2];
        stack[stack_pos - 1] = stack[stack_pos - 3];
    }
    else if (!strcmp(word, "+")) {
        printf ("%d + %d\n", stack[stack_pos], stack[stack_pos - 1]);
        stack[stack_pos - 1] += stack[stack_pos];
        stack_pos--;
    }
    else if (!strcmp(word, "*")) {
        stack[stack_pos - 1] *= stack[stack_pos];
        stack_pos--;
    }
    else if (!strcmp(word, "-")) {
        printf ("%d - %d\n", stack[stack_pos], stack[stack_pos - 1]);
        stack[stack_pos - 1] = stack[stack_pos] - stack[stack_pos - 1];
        stack_pos--;
    }
    else if (!strcmp(word, "/")) {
        stack[stack_pos - 1] = stack[stack_pos] / stack[stack_pos - 1];
        stack_pos--;
    }
    else if (!strcmp(word, "%")) {
        stack[stack_pos - 1] = stack[stack_pos] % stack[stack_pos - 1];
        stack_pos--;
    }
    else {
        if (push_if_number (word))
            printf ("push %d\n", stack[stack_pos]);
        else {
            user_def = find_def(word);
            if (NULL != user_def) {
                old_if_mode = if_mode;
                if_mode = NO_IF;
                eval_def (user_def);
                if_mode = old_if_mode;
            }
            else {
                printf ("Unknown word: %s\n", word);
            }
        }
    }
}


int push_if_number(const char* word) {
    int num;
    int num_read = sscanf (word, "%d", &num);
    if (1 == num_read) {
        stack_pos++;
        stack[stack_pos] = num;
        return 1;
    }
    else
        return 0;
}


void print_defs() {
    
    struct def_list *temp;
    struct word_list *words;
    
    if (NULL == defs_list) {
        printf ("No defs.\n");
    }
    else {
        printf ("Defs:\n");
        temp = defs_list;
        while (NULL != temp) {
            words = temp->def;
            while (NULL != words) {
                printf ("%s ", words->word);
                words = words->next;
            }
            printf ("\n");
            temp = temp->next;
        }
    }

}


void append_current_def() {

    struct def_list *new_node = (struct def_list *) malloc (sizeof (struct def_list));
    new_node->def = current_list;
    new_node->next = NULL;

    if (NULL == defs_list) {
        defs_list = new_node;
        end_defs_list = new_node;
    }
    else {
        end_defs_list->next = new_node;
        end_defs_list = new_node;
    }

}


void append_word_to_current_def(const char *string) {

    struct word_list *new_node = (struct word_list *) malloc (sizeof (struct word_list));
    new_node->word = (char *) malloc (sizeof (string) + 1);
    strcpy (new_node->word, string);
    new_node->next = NULL;

    if (NULL == current_list) {
        current_list = new_node;
        end_current_list = new_node;
    }
    else {
        end_current_list->next = new_node;
        end_current_list = new_node;
    }

    printf ("new word: %s\n", end_current_list->word);
}


void get_next_word() {
    char *p;
    int i;

    while (*buf_pos != (char) NULL && isspace(*buf_pos)) buf_pos++; // skip all whitespace

    // while at end of input buffer, read line into buf and skip whitespace
    while (*buf_pos == (char) NULL) {
        printf (">");
        if (fgets(buf, BUFSIZ, stdin) != NULL)
        {
            if ((p = strchr(buf, '\n')) != NULL)    *p = '\0';  // overwrite trailing newline char

            buf_pos = buf;  // reset current position
            while (*buf_pos != (char) NULL && isspace(*buf_pos)) buf_pos++; // skip all whitespace
        }
        else
            return;
    }
    // copy up to WORD_SIZE - 1 non-null, non-space characters into current_word and append NULL
    for (i = 0; i < WORD_SIZE - 1 && *buf_pos != (char) NULL && !isspace(*buf_pos); i++, buf_pos++) {
        current_word[i] = *buf_pos;
    }
    current_word[i] = (char) NULL;
}

void read_string() {
    int i;

    if (*buf_pos != (char) NULL && isspace(*buf_pos)) buf_pos++; // skip first whitespace

    // read only up to end of buffer/line or first NULL or first " (not \")
    for (i = 0; 
         i < WORD_SIZE - 1 && *buf_pos != (char) NULL && !('"' == *buf_pos && '\\' != *(buf_pos - 1)); 
         i++, buf_pos++) {
        current_string[i] = *buf_pos;
    }
    current_string[i] = (char) NULL;

    if ('"' == *buf_pos) buf_pos++; // skip over last "
}

