#include <am.h>
#include <amdev.h>
#include <klib.h>  //hjh
void splash();
void print_key();
void draw_ship();
static inline void puts(const char *s) 
{
  for (; *s; s++) _putc(*s);
}
typedef struct 
{
  int x, y, Side, Color, Speed;
}_ship;
typedef struct 
{
	int ship_x, ship_y, ship_side, ship_speed;
	uint32_t ship_color;
}_settings;
