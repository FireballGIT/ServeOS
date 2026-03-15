#include <stdint.h>

#define VGA  ((volatile uint16_t*)0xB8000)
#define COLOR 0x02
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define KEY_UP    1
#define KEY_DOWN  2
#define KEY_LEFT  3
#define KEY_RIGHT 4
#define KEY_ENTER '\n'
#define KEY_BACK  '\b'

volatile uint16_t cursor = 0;
uint8_t shift_pressed = 0;

// Shell command history
#define HISTORY_SIZE 10
char history[HISTORY_SIZE][64];
int history_count = 0;
int history_pos = -1;

// ======= VGA HELPERS =======
void scroll() {
    for(int i=SCREEN_WIDTH;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++) VGA[i-SCREEN_WIDTH]=VGA[i];
    for(int i=SCREEN_WIDTH*(SCREEN_HEIGHT-1);i<SCREEN_WIDTH*SCREEN_HEIGHT;i++) VGA[i]=(COLOR<<8)|' ';
    cursor -= SCREEN_WIDTH;
}

void putchar(char c){
    if(c=='\n') cursor += SCREEN_WIDTH - (cursor%SCREEN_WIDTH);
    else if(c==KEY_BACK){ if(cursor>0){ cursor--; VGA[cursor]=(COLOR<<8)|' '; } }
    else VGA[cursor++] = (COLOR<<8)|c;
    if(cursor >= SCREEN_WIDTH*SCREEN_HEIGHT) scroll();
}

void print(const char* str){ while(*str) putchar(*str++); }
void clear_screen(){ for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++) VGA[i]=(COLOR<<8)|' '; cursor=0; }

// ======= KEYBOARD =======
char get_ascii(uint8_t sc){
    static char normal[] = {
        0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,
        '\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };
    static char shifted[] = {
        0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
        0,'A','S','D','F','G','H','J','K','L',':','"','~',0,
        '|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
    };
    return sc<sizeof(normal)?(shift_pressed?shifted[sc]:normal[sc]):0;
}

uint8_t inb(uint16_t port){ uint8_t ret; __asm__ volatile("inb %1,%0":"=a"(ret):"Nd"(port)); return ret; }

uint8_t read_key(){
    static uint8_t last = 0;
    while(1){
        if(inb(0x64)&1){
            uint8_t sc = inb(0x60);
            if(sc!=last){
                last = sc;
                if(sc==0x2A || sc==0x36){ shift_pressed=1; continue; }
                if(sc==0xAA || sc==0xB6){ shift_pressed=0; continue; }
                if(sc==0xE0){ while(!(inb(0x64)&1)); uint8_t sc2=inb(0x60);
                    if(sc2==0x48) return KEY_UP;
                    if(sc2==0x50) return KEY_DOWN;
                    if(sc2==0x4B) return KEY_LEFT;
                    if(sc2==0x4D) return KEY_RIGHT;
                }
                if(!(sc & 0x80)){
                    char c = get_ascii(sc);
                    if(c) return c;
                }
            }
        }
    }
}

// ======= UTILS =======
int strcmp(const char* a,const char* b){ while(*a && (*a==*b)){a++;b++;} return *(unsigned char*)a - *(unsigned char*)b; }
void strcpy(char* d,const char* s){ while((*d++=*s++)); }
void itoa(int val,char* buf){ int i=0,neg=0;if(val<0){neg=1;val=-val;} char t[16];do{t[i++]='0'+(val%10);val/=10;}while(val);int j=0;if(neg)buf[j++]='-';while(i--)buf[j++]=t[i];buf[j]='\0'; }
int parse_expr(const char* str,int* a,char* op,int* b){ int i=0;*a=0;*b=0;*op=0;int neg=0;if(str[i]=='-'){neg=1;i++;}while(str[i]>='0'&&str[i]<='9') {*a=*a*10+(str[i]-'0'); i++;} if(neg)*a=-*a; if(str[i]=='+'||str[i]=='-'||str[i]=='*'||str[i]=='/'){*op=str[i];i++;} else return 0; neg=0; if(str[i]=='-'){neg=1;i++;}while(str[i]>='0'&&str[i]<='9') {*b=*b*10+(str[i]-'0');i++;} if(neg)*b=-*b; if(str[i]!='\0')return 0; return 1; }

// ======= WINDOWED APPS =======
void notepad(const char* filename){
    clear_screen();
    char notes[16][64]; int line_count=0;
    print("[NOTEPAD: "); print(filename); print("] ::exit to quit\n");
    while(1){
        char line[64]; int i=0;
        while(1){ char k=read_key(); if(k==KEY_ENTER){ putchar('\n'); line[i]='\0'; break; } else if(k==KEY_BACK){ if(i>0){ i--; putchar(KEY_BACK); } } else if(i<63){ line[i++]=k; putchar(k); } }
        if(strcmp(line,"::exit")==0) break;
        if(line_count<16) strcpy(notes[line_count++],line);
        else print("[Memory full]\n");
    }
    clear_screen();
}

void calculator(){
    clear_screen();
    print("[Calculator] e.g. 2+3, ::exit to quit\n");
    while(1){
        char input[16]; int i=0;
        while(1){ char k=read_key(); if(k==KEY_ENTER){ putchar('\n'); input[i]='\0'; break; } else if(k==KEY_BACK){ if(i>0){ i--; putchar(KEY_BACK); } } else if(i<15){ input[i++]=k; putchar(k); } }
        if(strcmp(input,"::exit")==0) break;
        int a,b,res=0; char op;
        if(parse_expr(input,&a,&op,&b)){
            if(op=='+') res=a+b;
            else if(op=='-') res=a-b;
            else if(op=='*') res=a*b;
            else if(op=='/') res=b? a/b:0;
            char buf[16]; itoa(res,buf); print(buf); putchar('\n');
        } else print("[Invalid]\n");
    }
    clear_screen();
}

void compass(){
    clear_screen();
    const char* files[]={"kernel.bin","boot.flp","readme.txt"};
    int file_count=3; int sel=0;
    print("[Compass Explorer] Arrow keys to move, Enter to open, ::exit to quit\n");
    while(1){
        for(int i=0;i<file_count;i++){ if(i==sel) print("> "); else print("  "); print(files[i]); putchar('\n'); }
        uint8_t k=read_key();
        if(k==KEY_ENTER) notepad(files[sel]);
        else if(k==KEY_UP && sel>0) sel--;
        else if(k==KEY_DOWN && sel<file_count-1) sel++;
        else if(k==':'){ uint8_t n=read_key(); if(n==':'){ uint8_t e=read_key(); if(e=='e') break; } }
        clear_screen();
    }
}

// ======= SNAKE =======
#define SNAKE_MAX 100
typedef struct{ int x,y; } Point;
void snake(){
    clear_screen();
    print("[Snake] Arrow keys to move, ::exit to quit\n");

    Point snake_body[SNAKE_MAX]; 
    int len=3;
    snake_body[0].x=10; snake_body[0].y=5;
    snake_body[1].x=9; snake_body[1].y=5;
    snake_body[2].x=8; snake_body[2].y=5;
    Point fruit = {15,10};
    int dx=1, dy=0;
    int temp_dx=1, temp_dy=0;

    while(1){
        // Draw
        clear_screen();
        for(int i=0;i<len;i++){ 
            int idx=snake_body[i].y*SCREEN_WIDTH+snake_body[i].x; 
            if(idx<SCREEN_WIDTH*SCREEN_HEIGHT)VGA[idx]=(COLOR<<8)|'O'; 
        }
        int fidx=fruit.y*SCREEN_WIDTH+fruit.x; 
        VGA[fidx]=(COLOR<<8)|'F';

        // Input
        uint8_t k=read_key();
        if(k==KEY_UP && dy!=1){ temp_dx=0; temp_dy=-1; }
        else if(k==KEY_DOWN && dy!=-1){ temp_dx=0; temp_dy=1; }
        else if(k==KEY_LEFT && dx!=1){ temp_dx=-1; temp_dy=0; }
        else if(k==KEY_RIGHT && dx!=-1){ temp_dx=1; temp_dy=0; }
        else if(k==':'){ uint8_t n=read_key(); if(n==':'){ uint8_t e=read_key(); if(e=='e') break; } }

        // Move
        dx = temp_dx;
        dy = temp_dy;
        Point new_head = {snake_body[0].x+dx, snake_body[0].y+dy};
        for(int i=len;i>0;i--) snake_body[i]=snake_body[i-1];
        snake_body[0]=new_head;

        // Check fruit
        if(new_head.x==fruit.x && new_head.y==fruit.y){ 
            if(len<SNAKE_MAX) len++; 
            fruit.x=(fruit.x+7)%SCREEN_WIDTH; 
            fruit.y=(fruit.y+5)%SCREEN_HEIGHT; 
        }
    }

    clear_screen();
}


// ======= SHELL =======
void shell(){
    char cmd[64]; int i;
    while(1){
        print("UM// >"); i=0; history_pos=history_count;
        while(1){
            uint8_t k=read_key();
            if(k==KEY_UP){ if(history_pos>0){ history_pos--; strcpy(cmd,history[history_pos]); clear_screen(); print("SRV//> "); print(cmd); i=0; while(cmd[i]){ putchar(cmd[i]); i++; } } continue; }
            if(k==KEY_DOWN){ if(history_pos<history_count-1){ history_pos++; strcpy(cmd,history[history_pos]); clear_screen(); print("SRV//> "); print(cmd); i=0; while(cmd[i]){ putchar(cmd[i]); i++; } } continue; }
            if(k==KEY_ENTER){ putchar('\n'); break; }
            else if(k==KEY_BACK){ if(i>0){ i--; putchar(KEY_BACK); } }
            else if(i<63){ cmd[i++]=k; putchar(k); }
        }
        cmd[i]='\0';
        if(history_count<HISTORY_SIZE) strcpy(history[history_count++],cmd);
        else { for(int h=1;h<HISTORY_SIZE;h++) strcpy(history[h-1],history[h]); strcpy(history[HISTORY_SIZE-1],cmd); }

        if(strcmp(cmd,"exit")==0){ print("System Halted\n"); while(1)__asm__("hlt"); }
        else if(strcmp(cmd,"halt")==0){ print("System Halted\n"); while(1)__asm__("hlt"); }
        else if(strcmp(cmd,"notepad")==0) notepad("Untitled");
        else if(strcmp(cmd,"calc")==0) calculator();
        else if(strcmp(cmd,"compass")==0) compass();
        else if(strcmp(cmd,"snake")==0) snake();
        else if(cmd[0]=='e' && cmd[1]=='c' && cmd[2]=='h' && cmd[3]=='o' && cmd[4]==' '){ print(cmd+5); putchar('\n'); }
        else if(strcmp(cmd,"ls")==0) print("kernel.bin\nboot.flp\nreadme.md\n");
        else { print("[1551] ):\nThe command '"); print(cmd); print("' isn't valid. Try again!"); }
    }
}

// ======= KERNEL =======
void splash() {
    print("BBBBB    AAA     GGGGG   EEEEE   L        OOOOO    SSSSS\n");
    print("B    B  A   A   G        E       L       O     O  S\n");
    print("BBBBB   AAAAA   G  GGG   EEEE    L       O     O   SSSS\n");
    print("B    B  A   A   G    G   E       L       O     O       S\n");
    print("BBBBB   A   A    GGGG    EEEEE   LLLLL    OOOOO   SSSSS\n");
}
void kernel_main() { clear_screen(); splash(); print("Running on BK2\n"); shell(); }
