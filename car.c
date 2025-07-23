#include<reg51.h>
#include "font.h"

#define ROAD_POINTS_MAX 8
#define OBSTACLES_MAX 4

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

unsigned int seed;
unsigned int distance;
unsigned int last_distance;
unsigned char game_over_flag;
unsigned char win_flag;
unsigned char tunnel_flag;

unsigned char player_x;
unsigned char player_last_x;
unsigned char player_speed;
unsigned char player_lives;

unsigned char road_radius;
unsigned char road_last_min_x[8];
unsigned char road_last_max_x[8];
unsigned char road_points_x[ROAD_POINTS_MAX];
unsigned int road_points_y[ROAD_POINTS_MAX];

unsigned char obstacles_x[OBSTACLES_MAX];
unsigned int obstacles_y[OBSTACLES_MAX];

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

void GLCD_set_column(unsigned char col) {
    if (col < 64)
        GLCD_write_cmd(0x40 + col);
}

void GLCD_set_row(unsigned char row) {
    if (row < 8)
        GLCD_write_cmd(0xB8 + row);
}

void GLCD_write_char(char code *ptr_array) {
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
    return a + (seed % (b - a));
}

void hud_update_distance() {
    char buf[6];
    GLCD_select_panel(1);
    GLCD_set_row(2);
    GLCD_set_column(34);
    int_to_str5(distance, buf);
    GLCD_write_string(buf);
}

void hud_update_speed() {
    GLCD_select_panel(1);
    GLCD_set_row(3);
    GLCD_set_column(58);
    GLCD_write_char(get_char(int_to_str1(player_speed)));
}

void hud_update_lives() {
    GLCD_select_panel(1);
    GLCD_set_row(4);
    GLCD_set_column(58);
    GLCD_write_char(get_char(int_to_str1(player_lives)));
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

    hud_update_distance();
    hud_update_speed();
    hud_update_lives();
}

int collision_check(unsigned char x) {
    if (x >= player_x && x <= player_x + 6) {
        return 1;
    } else {
        return 0;
    }
}

void road_new_segment() {
    int i;

    for (i = 1; i < ROAD_POINTS_MAX; i++) {
        road_points_x[i-1] = road_points_x[i];
        road_points_y[i-1] = road_points_y[i];
    }

    road_points_x[ROAD_POINTS_MAX - 1] = rand_range(road_radius, 63 - road_radius);
    road_points_y[ROAD_POINTS_MAX - 1] = road_points_y[ROAD_POINTS_MAX - 2] + 60 + (rand() & 0x3f);
}

void road_update() {
    last_distance = distance;
    distance += player_speed;
    hud_update_distance();

    if (road_points_y[1] < distance) {
        road_new_segment();
    }
}

// Assume 8-bit fixed point with 0–255 range for t (i.e., t ∈ [0, 255])
unsigned char lerp(unsigned char a, unsigned char b, unsigned char t) {
    return a + (((b - a) * t) >> 8);  // Fast linear interpolation
}

// interpola x entre um ponto e o ponto seguinte da entrada
// y em coordenadas do universo
unsigned char road_get_x(unsigned int y, unsigned char point) {
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

unsigned char find_road_segment(unsigned int y) {
    unsigned char seg;
    for (seg = 0; seg < ROAD_POINTS_MAX - 1; seg++) {
        if (y > road_points_y[seg] && y <= road_points_y[seg+1]) {
            return seg;
        }
    }
    return ROAD_POINTS_MAX - 2;  // fallback: last segment
}

void obstacle_respawn() {
    unsigned int i, y, road;

    for (i = 1; i < OBSTACLES_MAX; i++) {
        obstacles_x[i-1] = obstacles_x[i];
        obstacles_y[i-1] = obstacles_y[i];
    }

    y = obstacles_y[OBSTACLES_MAX - 2] + 60 + (rand() & 0x3f);
    road = (unsigned int)road_get_x(y, find_road_segment(y));

    obstacles_x[OBSTACLES_MAX - 1] = (unsigned char)rand_range(road - road_radius + 2, road + road_radius - 8);
    obstacles_y[OBSTACLES_MAX - 1] = y;
}

int f1(int x, int y) {
    int x1, C;
    x1 = player_x + 6;

    // Compute C
    C = 705 + 8*x1;

    // Plug in implicit line
    return -8 * x - 15 * y + C;
}

int f1_prime(int x, int y) {
    int x1, y1, dx, dy, b, C;
    x1 = player_x;
    y1 = 63 - 16;
    dx = -15;
    dy = -8;

    // Compute C
    C = dx*y1 - dy*x1;

    // Plug in implicit line
    return dy * x - dx * y + C;
}

unsigned char get_mask_xy(unsigned char x, unsigned char y, unsigned char page) {
    if (page == 7 || page < 2) {
        return 1;
    }

    else if (page == 2) {
        return (f1(x, y) < 0 || f1_prime(x, y) > 0);
    }

    return 0;
}

unsigned char byte_mask(unsigned char x, unsigned char page) {
    unsigned char byte = 0, b;

    for (b = 0; b < 8; b++) {
        if (get_mask_xy(x, (7 - page) * 8 + b, page)) {
            byte |= 0x80 >> b;
        }
    }

    return byte;
}

void GLCD_write_char_xy(unsigned char x, unsigned char y, unsigned char code *ptr_array) {
    int i;
    unsigned char page = y >> 3;
    unsigned char offset = y & 0x07;
    unsigned char byte;

    if (page < 8){
        GLCD_set_row(7 - page);
        GLCD_set_column(x);
        for(i=0;i<6;i++) { // 5x7 font, 5 chars + 1 blankspace
            byte = ptr_array[i] << (8 - offset);
            if (tunnel_flag) byte |= byte_mask(x + i, 7 - page);
            GLCD_write_data(byte);
        }
    }

    if (page > 0) {
        GLCD_set_row(8 - page);
        GLCD_set_column(x);
        for(i=0;i<6;i++) { // 5x7 font, 5 chars + 1 blankspace
            byte = ptr_array[i] >> offset;
            if (tunnel_flag) byte |= byte_mask(x + i, 8 - page);
            GLCD_write_data(byte);
        }
    }
}

void crash() {
    GLCD_select_panel(0);
	GLCD_set_row(6);
	GLCD_set_column(player_x);
    GLCD_write_char(crash_sprite);
    
    if (player_lives == 0) {
        game_over_flag = 1;
        return;
    }
    player_lives--;
    hud_update_lives();

    delay_ms(400);
    
    GLCD_select_panel(0);

    if (obstacles_y[0] - distance < 40) {
        GLCD_write_char_xy(obstacles_x[0], obstacles_y[0] - distance, space);
        obstacle_respawn();
    }

    GLCD_set_row(6);
    GLCD_set_column(player_x);
    GLCD_write_char(space);
    player_x = road_get_x(distance + 16, find_road_segment(distance + 16)) - 3;
    GLCD_set_column(player_x);
    GLCD_write_char(car_sprite);
    
    delay_ms(200);
}

void tunnel_draw() {
    unsigned char page, x;

    GLCD_select_panel(0);

    for (page = 0; page < 8; page++) {
		GLCD_set_row(page);
        GLCD_set_column(0);
        for (x = 0; x < 64; x++) {
			GLCD_write_data(byte_mask(x, page));
        }
    }
}

void obstacles_update() {
    if (obstacles_y[0] < distance) {
        obstacle_respawn();
    }
}

void obstacles_draw() {
    unsigned int y, last_y;
    unsigned char i;

    GLCD_select_panel(0);

    for (i = 0; i < OBSTACLES_MAX; i++) {
        y = obstacles_y[i] - distance;
        last_y = obstacles_y[i] - last_distance;

        if (last_y > 0 && last_y <= 71) GLCD_write_char_xy(obstacles_x[i], last_y, space);
        if (y > 0 && y <= 71) GLCD_write_char_xy(obstacles_x[i], y, obstacle_sprite);

        if (y > 8 && y < 24) {
            if (collision_check(obstacles_x[i]) || collision_check(obstacles_x[i] + 6)) {
                GLCD_write_char_xy(obstacles_x[i], y, crash_sprite);
                crash();
            }
        }
    }
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
        if (tunnel_flag) byte |= byte_mask(i, page);
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
        if (tunnel_flag) byte |= byte_mask(i, page);
        GLCD_write_data(byte);
    }
}

// desenha um frame da estrada
void road_draw() {
    unsigned char page, seg, b;
    unsigned int y = distance;
    unsigned char road_x[8];  // Pre-calculate positions

    seg = 0;

    GLCD_select_panel(0);

    for(page = 0; page < 8; page++) {
        for (b = 0; b < 8; b++) {
            road_x[b] = road_get_x(y, seg);
            y++;

            if (y >= road_points_y[seg+1]) {  // hit next segment
                seg++;
            }
            
            if (page == 1){
                if (collision_check(road_x[b] - road_radius) || collision_check(road_x[b] + road_radius)) {
                    crash();
                }
            }
        }
        road_draw_page(7 - page, road_x);
    }
}

void player_draw() {
    if (player_x != player_last_x) {
        GLCD_select_panel(0);
        GLCD_set_row(6);	    /* Set page */
        GLCD_set_column(player_last_x);	/* Set column */
        GLCD_write_char(space);
        GLCD_set_column(player_x);	/* Set column */
        GLCD_write_char(car_sprite);
    }
}

void player_update() {
    char input;
    player_last_x = player_x;
    input = serial_receive();
    switch (input) {
        case '7':
            if (player_x <= 3) {
                player_x = 0;
            } else {
                player_x -= 3;
            }
            break;
        case '9':
            player_x += 3;
            if (player_x >= 58) {
                player_x = 58;
            }
            break;
        case '1':
            player_speed = 1;
            break;
        case '2':
            player_speed = 2;
            break;
        case '3':
            player_speed = 3;
            break;
        case '4':
            player_speed = 4;
            break;
        case '5':
            player_speed = 5;
            break;
        case '6':
            crash();
            break;
    }
    hud_update_speed();
}

void game_update() {
    player_draw();
    obstacles_draw();
    road_draw();
    player_update();
    obstacles_update();
    road_update();

    // if (distance >= 100 && !tunnel_flag) {
    //     tunnel_flag = 1;
    //     tunnel_draw();
    // }

    if (distance >= 15000) win_flag = 1;
}

void player_init() {
    player_x = 29;
    player_speed = 3;
    player_lives = 9;
}

void road_init() {
    int i;

    road_radius = 18;
    for (i = 0; i < 8; i++) {
        road_last_min_x[i] = 63;
        road_last_max_x[i] = 0;
    }

    road_points_x[ROAD_POINTS_MAX - 2] = 31;
    road_points_x[ROAD_POINTS_MAX - 1] = 31;

    road_points_y[ROAD_POINTS_MAX - 2] = 0;
    road_points_y[ROAD_POINTS_MAX - 1] = 80;
    
    for (i = 0; i < ROAD_POINTS_MAX - 2; i ++) {
        road_new_segment();
    }
}

void obstacles_init() {
    unsigned char i;

    obstacles_x[OBSTACLES_MAX - 1] = 40;
    obstacles_y[OBSTACLES_MAX - 1] = 60;
    
    for (i = 0; i < OBSTACLES_MAX; i ++) {
        obstacle_respawn();
    }
}

void game_init() {
    distance = 0;
    last_distance = 0;
    game_over_flag = 0;
    win_flag = 0;
    player_init();
    road_init();
    obstacles_init();
    GLCD_clear_screen();
    player_draw();
    road_draw();
    hud_draw();
}

void game_over() {
    GLCD_select_panel(0);
    GLCD_set_row(2);
    GLCD_set_column(20);
    GLCD_write_string("GAME");
    GLCD_set_row(3);
    GLCD_set_column(20);
    GLCD_write_string("OVER");
    game_over_flag = 0;
    delay_ms(1000);
    RI = 0;

    while (!(serial_receive()));
    
    GLCD_set_row(2);
    GLCD_set_column(20);
    GLCD_write_string("    ");
    GLCD_set_row(3);
    GLCD_set_column(20);
    GLCD_write_string("    ");
}

void win() {
    GLCD_select_panel(0);
    GLCD_set_row(2);
    GLCD_set_column(4);
    GLCD_write_string("WELL DONE");
    GLCD_set_row(3);
    GLCD_set_column(10);
    GLCD_write_string("YOU WIN");
    win_flag = 0;
    delay_ms(1000);
    RI = 0;

    while (!(serial_receive()));
    
    GLCD_set_row(2);
    GLCD_set_column(4);
    GLCD_write_string("         ");
    GLCD_set_row(3);
    GLCD_set_column(10);
    GLCD_write_string("       ");
}

void title() {
    GLCD_select_panel(0);
    GLCD_set_row(2);
    GLCD_set_column(4);
    GLCD_write_string("STOCK CAR");

    while (!(serial_receive())) {
        seed++;
    }

    GLCD_set_column(4);
    GLCD_write_string("         ");
}

int main() {
    seed = 10;
    serial_init();
    GLCD_init();
    while(1) {
        game_init();
        title();

        while (1) {
            game_update();
            delay_ms(12);

            if (game_over_flag) {
                game_over();
                break;
            }

            if (win_flag) {
                win();
                break;
            }
        }
    }
}
