#include<reg51.h>
#include "font.h"

#define ROAD_POINTS_MAX 6
#define ENEMIES_MAX 4

/* Configure the data bus and Control bus as per the hardware connection
   Dtatus bus is connected to P20:P27 and control bus P00:P04*/
#define GlcdDataBus  P2
sbit RS  = P1^0;
sbit EN  = P1^1;
sbit CS1 = P1^2;
sbit CS2 = P1^3;
sbit RW  = P1^4;
sbit RST = P1^5; // Assuming unused pin

// Keypad
sbit R1 = P0^0;
sbit R2 = P0^1;
sbit R3 = P0^2;
sbit R4 = P0^3;
sbit C1 = P0^4;
sbit C2 = P0^5;
sbit C3 = P0^6;

typedef struct {
    unsigned char x;
    unsigned char last_x;
    unsigned char speed;
    unsigned char lives;
} Player;

Player player;

unsigned int seed;
unsigned int distance;
unsigned char road_radius;
unsigned char road_last_min_x[8];
unsigned char road_last_max_x[8];
unsigned char road_points_x[ROAD_POINTS_MAX];
unsigned int road_points_y[ROAD_POINTS_MAX];
unsigned char road_points_t_step[ROAD_POINTS_MAX];

unsigned char enemies_x[ENEMIES_MAX];
unsigned char enemies_y[ENEMIES_MAX];

void delay(unsigned int count) {
	int i;
	for(i = 0; i < count; i++);
}

void delay_ms(unsigned int count) {
	int i, j;
	for(i = 0; i < count; i++)
	for(j = 0; j < 112; j++);
}

void serial_init(void) {
    TMOD = 0x20;       // Timer1, modo 2 (auto-reload de 8 bits)
    TH1 = 0xFD;        // Baud rate 9600
    SCON = 0x50;       // Modo 1, 8 bits, REN habilitado
    TR1 = 1;           // Inicia Timer1
    ES = 0;            // Habilita interrup��o serial
    EA = 0;            // Habilita interrup��es globais
}

char serial_receive(void) {
    if (!RI) return 0;  // No data
    RI = 0;
    return SBUF;
}

void serial_send(char dat) {
    TI = 0;
    SBUF = dat;
    while (TI == 0);
}

// char keypad_read()
// {
// 	C1=1;
// 	C2=1;
// 	C3=1;

// 	R1=0;
// 	R2=1;
// 	R3=1;
// 	R4=1;
// 	if(C1==0){delay_ms(50);return '3';}
// 	if(C2==0){delay_ms(50);return '2';}
// 	if(C3==0){delay_ms(50);return '1';}
// 	R1=1;
// 	R2=0;
// 	R3=1;
// 	R4=1;
// 	if(C1==0){delay_ms(50);return '6';}
// 	if(C2==0){delay_ms(50);return '5';}
// 	if(C3==0){delay_ms(50);return '4';}
// 	R1=1;
// 	R2=1;
// 	R3=0;
// 	R4=1;
// 	if(C1==0){delay_ms(50);return '9';}
// 	if(C2==0){delay_ms(50);return '8';}
// 	if(C3==0){delay_ms(50);return '7';}
// 	R1=1;
// 	R2=1;
// 	R3=1;
// 	R4=0;
// 	if(C1==0){delay_ms(50);return '#';}
// 	if(C2==0){delay_ms(50);return '0';}
// 	if(C3==0){delay_ms(50);return '*';}
// 	return 0;
// }

void GLCD_select_panel(int panel) {
    switch (panel) {
        case 0:
            CS1 = 0;
            CS2 = 1;
            break;
        case 1:
            CS1 = 1;
            CS2 = 0;
            break;
        case 2:
            CS1 = 0;
            CS2 = 0;
            break;
        default:
            return;
    }
}

/* Function to send the command to LCD */
void GLCD_write_cmd(char cmd)
{
    GlcdDataBus = cmd;   //Send the Command 
    RS = 0;              // Send LOW pulse on RS pin for selecting Command register
    RW = 0;              // Send LOW pulse on RW pin for Write operation
    EN = 1;              // Generate a High-to-low pulse on EN pin

    EN = 0;
}

/* Function to send the data to LCD */
void GLCD_write_data(char dat)
{
    GlcdDataBus = dat;   //Send the data on DataBus
    RS = 1;              // Send HIGH pulse on RS pin for selecting data register
    RW = 0;              // Send LOW pulse on RW pin for Write operation
    EN = 1;              // Generate a High-to-low pulse on EN pin

	EN = 0;
}

void GLCD_write_char(char code *ptr_array)
{
    int i;
    for(i=0;i<6;i++) // 5x7 font, 5 chars + 1 blankspace
        GLCD_write_data(ptr_array[i]);
}

const unsigned char code *get_char(char c) {
    // Digits
    if (c >= '0' && c <= '9') {
        return font_digits[c - '0'];
    }

    else if (c >= 'A' && c <= 'Z') {
        return font_uppercase[c - 'A'];
    }

    else if (c == ':') {
        return colon;
    }

    else if (c == '.') {
        return period;
    }

    // Default: space
    return space;
}

void GLCD_write_string(char *text) {
    while (*text) {
        const unsigned char code *chr = get_char(*text++);
        GLCD_write_char(chr);
    }
}

void GLCD_init()		/* GLCD initialize function */
{
	CS1 = 0; CS2 = 0;	/* Select left & right half of display */
	RST = 1;		/* Keep reset pin high */	
	delay_ms(20);
	GLCD_write_cmd(0x3E);	/* Display OFF */
	GLCD_write_cmd(0x40);	/* Set Y address (column=0) */
	GLCD_write_cmd(0xB8);	/* Set x address (page=0) */
	GLCD_write_cmd(0xC0);	/* Set z address (start line=0) */
	GLCD_write_cmd(0x3F);	/* Display ON */
}

void GLCD_clear_screen()		/* GLCD all display clear function */
{
	int i,j;
	CS1 = 0; CS2 = 0;	/* Select left & right half of display */
	for(i=0;i<8;i++)
	{
		GLCD_write_cmd((0xB8)+i);	/* Increment page */
		for(j=0;j<64;j++)
		{
			GLCD_write_data(0);	/* Write zeros to all 64 column */
		}
	}
	GLCD_write_cmd(0x40);	/* Set Y address (column=0) */
	GLCD_write_cmd(0xB8);	/* Set x address (page=0) */
}

void GLCD_set_column(unsigned char col) {
    if (col < 64)
    GLCD_write_cmd(0x40 + col);
}

void GLCD_set_row(unsigned char row) {
    GLCD_write_cmd(0xB8 + row);
}

// void GLCD_scroll_screen(unsigned char z) {
// 	GLCD_write_cmd(0xc0 + z);
// }

void int_to_str5(unsigned int v, char *buf) {
    buf[0] = '0' + ((v / 10000) % 10);
    buf[1] = '0' + ((v / 1000) % 10);
    buf[2] = '0' + ((v / 100) % 10);
    buf[3] = '0' + ((v / 10) % 10);
    buf[4] = '0' + (v % 10);
    buf[5] = '\0';
}

char int_to_str1(unsigned char v) {
    return '0' + (v % 10);
}

unsigned int rand() {
    seed = seed * 1103515245 + 12345;
    return seed;
}

unsigned int rand_range(unsigned int a, unsigned int b) {
    seed = seed * 1103515245 + 12345;
    return a + seed % (b - a);
}

// Simple hash function for deterministic "randomness"
unsigned int simple_hash(unsigned int x) {
    x ^= x >> 13;
    x *= 0x45d9f3b;
    // x += distance;
    x ^= x >> 16;
    return x;
}

void hud_update_distance() {
    char buf[6];
    GLCD_select_panel(1);
    GLCD_set_row(2);
    GLCD_set_column(33);
    int_to_str5(distance, buf);
    GLCD_write_string(buf);
}

void hud_update_speed() {
    GLCD_select_panel(1);
    GLCD_set_row(3);
    GLCD_set_column(57);
    GLCD_write_char(get_char(int_to_str1(player.speed)));
}

void hud_update_lives() {
    GLCD_select_panel(1);
    GLCD_set_row(4);
    GLCD_set_column(57);
    GLCD_write_char(get_char(int_to_str1(player.lives)));
}

void hud_draw() {
    GLCD_select_panel(1);
    GLCD_set_row(2);
    GLCD_set_column(0);
    GLCD_write_string("DIST.");

    GLCD_set_row(3);
    GLCD_set_column(0);
    GLCD_write_string("SPEED");

    
    GLCD_set_row(4);
    GLCD_set_column(0);
    GLCD_write_string("LIVES");

    hud_update_speed();
    hud_update_lives();
}


int collision_check(unsigned char x) {
    if (x >= player.x && x <= player.x + 6) {
        return 1;
    } else {
        return 0;
    }
}

void game_over() {

}

void crash(unsigned char x) {
    player.lives--;
    if (player.lives < 0) {
        game_over();
    }
    hud_update_lives();
    GLCD_select_panel(0);
	GLCD_set_row(6);	    /* Set page */
	GLCD_set_column(player.x);	/* Set column */
    GLCD_write_char(crash_sprite);
    delay_ms(400);
    player.x = x - 3;
}

void enemy_respawn() {
    unsigned char x;
    unsigned int i, y;

    for (i = 1; i < ENEMIES_MAX; i++) {
        enemies_x[i-1] = enemies_x[i];
        enemies_y[i-1] = enemies_y[i];
    }

    // y = enemies_y[ENEMIES_MAX - 1] +;
}

void enemies_update() {
    unsigned char i;

    for (i = 0; i < ENEMIES_MAX; i++) {
        // enemies_y[]
    }
}

void road_new_segment() {
    unsigned char x;
    int seg_len, i;

    for (i = 1; i < ROAD_POINTS_MAX; i++) {
        road_points_x[i-1] = road_points_x[i];
        road_points_y[i-1] = road_points_y[i];
        road_points_t_step[i-1] = road_points_t_step[i];
    }

    road_points_x[ROAD_POINTS_MAX - 1] = rand_range(road_radius, 63 - road_radius);
    road_points_y[ROAD_POINTS_MAX - 1] = road_points_y[ROAD_POINTS_MAX - 2] + 60 + (rand() & 0x1f);

    seg_len = road_points_y[ROAD_POINTS_MAX - 1] - road_points_y[ROAD_POINTS_MAX - 2];
    road_points_t_step[ROAD_POINTS_MAX - 2] = 256 / seg_len;
    // serial_send(x);
    // serial_send(road_points_y[5]);
}

void road_update() {
    distance += player.speed + 1;
    hud_update_distance();

    if (road_points_y[1] < distance) {
        road_new_segment();
    }
}

// Assume 8-bit fixed point with 0–255 range for t (i.e., t ∈ [0, 255])
unsigned char lerp(unsigned char a, unsigned char b, unsigned char t) {
    return a + (((b - a) * t) >> 8);  // Fast linear interpolation
}

unsigned char road_get_x_from_y(unsigned int y, unsigned char point) {
    unsigned char x0, x1, seg_len, t;
    int y0, y1;

    x0 = road_points_x[point];
    x1 = road_points_x[point+1];
    
    y0 = road_points_y[point];
    y1 = road_points_y[point+1];

    seg_len = y1 - y0;
    t = ((y - y0) * 256) / seg_len;

    return lerp(x0, x1, t);
}

unsigned char road_get_x(unsigned char t, unsigned char point) {
    // unsigned char seg;
    unsigned char p0, p1;

    // Get control points for this segment
    p0 = road_points_x[point];
    p1 = road_points_x[point+1];

    // Return the interpolated X
    return lerp(p0, p1, t);

    // return abs((y % ((63 -  2 * road_radius) * 2)) - (63 - 2 * road_radius)) + road_radius;
}

void road_draw_page(unsigned char page, unsigned char road_x[8]) {
    unsigned char min_x = 63, max_x = 0, start, end;
    unsigned char i, b, byte;
    
    for (b = 0; b < 8; b++) {
        
        // Find min/max bounds for current page
        if (road_x[b] < min_x) min_x = road_x[b];
        if (road_x[b] > max_x) max_x = road_x[b];
    }

    start = (min_x < road_last_min_x[page]) ? min_x : road_last_min_x[page];
    end = (max_x > road_last_max_x[page]) ? max_x : road_last_max_x[page];

    road_last_min_x[page] = min_x;
    road_last_max_x[page] = max_x;
    
    GLCD_set_row(page);
    
    // Draw left edge (erase old + draw new)
    GLCD_set_column(start - road_radius);
    for (i = start; i <= end; i++) {
        byte = 0;
        for (b = 0; b < 8; b++) {
            // Set bit if this is current road position
            if (i == road_x[b]) {
                byte |= (0x80 >> b);
            }
        }
        GLCD_write_data(byte);
    }
    
    // Draw right edge (erase old + draw new)
    GLCD_set_column(start + road_radius);
    for (i = start; i <= end; i++) {
        byte = 0;
        for (b = 0; b < 8; b++) {
            // Set bit if this is current road position
            if (i == road_x[b]) {
                byte |= (0x80 >> b);
            }
        }
        GLCD_write_data(byte);
    }
}

void road_draw() {
    unsigned char page, t, t_step, seg, seg_len, b;
    unsigned int y = distance;
    unsigned char road_x[8];  // Pre-calculate positions

    seg = 0;
    // seg_len = road_points_y[seg+1] - road_points_y[seg];
    // t = ((y - road_points_y[seg]) * 256) / seg_len;
    // t_step = road_points_t_step[seg];

    GLCD_select_panel(0);

    for(page = 0; page < 8; page++) {
        for (b = 0; b < 8; b++) {
            road_x[b] = road_get_x_from_y(y, seg);
            y++;
            // t += t_step;

            if (y >= road_points_y[seg+1]) {  // hit next segment
                seg++;
                // t = 0;
                // t_step = road_points_t_step[seg];
            }
            if (page == 1){
                if (collision_check(road_x[b] - road_radius) || collision_check(road_x[b] + road_radius)) {
                    crash(road_x[b]);
                }
            }
        }
        road_draw_page(7 - page, road_x);
    }

    // delay_ms(800);
}

void player_draw() {
    if (player.x != player.last_x) {
        GLCD_select_panel(0);
        GLCD_set_row(6);	    /* Set page */
        GLCD_set_column(player.last_x);	/* Set column */
        GLCD_write_char(space);
        GLCD_set_column(player.x);	/* Set column */
        GLCD_write_char(car_sprite);
    }
}

void enemy_draw() {

}

void input_update() {
    char input;
    input = serial_receive();
    switch (input) {
        case '7':
            player.x -= 3;
            if (player.x <= 0) {
                player.x = 0;
            }
            break;
        case '9':
            player.x += 3;
            if (player.x >= 58) {
                player.x = 58;
            }
            break;
        case '1':
            player.speed = 1;
            break;
        case '2':
            player.speed = 2;
            break;
        case '3':
            player.speed = 3;
            break;
        case '4':
            player.speed = 4;
            break;
        case '5':
            player.speed = 5;
            break;
    }
    hud_update_speed();
}

void game_update() {
    input_update();
    player_draw();
    player.last_x = player.x;
    road_update();
    road_draw();
}

void player_init() {
    player.x = 29;
    player.speed = 3;
    player.lives = 9;
}

void road_init(unsigned int seed) {
    int i;

    road_radius = 15;
    distance = 0;
    for (i = 0; i < 8; i++) {
        road_last_min_x[i] = 63;
        road_last_max_x[i] = 0;
    }

    road_points_x[ROAD_POINTS_MAX - 2] = 31;
    road_points_x[ROAD_POINTS_MAX - 1] = 31;

    road_points_y[ROAD_POINTS_MAX - 2] = 0;
    road_points_y[ROAD_POINTS_MAX - 1] = 60;
    
    road_points_t_step[ROAD_POINTS_MAX - 2] = 256 / 60;
    
    for (i = 0; i < ROAD_POINTS_MAX - 2; i ++) {
        road_new_segment();
    }
}

void game_init() {
    player_init();
    road_init(10);
    GLCD_clear_screen();
    road_draw();
    player_draw();
}

int main() {
    serial_init();
    GLCD_init();
    game_init();
    hud_draw();
    while(1) {
        game_update();
        delay(100);
    }
}