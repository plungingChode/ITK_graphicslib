#include "graphics2.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "char3.h"
#include "keytbl.h"

using namespace genv;

groutput &genv::gout = groutput::instance();
grinput &genv::gin   = grinput::instance();

// Canvas
canvas::canvas() 
	: draw_color(0xFFFFFF), buf(NULL), 
		transparent(false), loaded_font(NULL)
{
}

canvas::canvas(int w, int h)
	: draw_color(0xFFFFFF), buf(NULL), 
		transparent(false), loaded_font(NULL)
{
	open(w, h);
}

canvas::canvas(const canvas &c) {
	*this = c;
	loaded_font = c.loaded_font; // doesn't get copied for some reason
}

canvas::~canvas() {
	if (buf) SDL_FreeSurface(buf);
	if (loaded_font) TTF_CloseFont(loaded_font);
}

Uint32& canvas::pixel(int x, int y) {
	Uint32 *pixels = (Uint32*)buf->pixels;
	return pixels[y*buf->w + x];
}

bool canvas::in_bounds(int x, int y) {
	return !(x < 0 || y < 0 || x >= buf->w || y >= buf->h);
}

void canvas::draw_line(int x, int y, bool relative) {
	if (!relative) {
		x -= draw_x;
		y -= draw_y;
	}

	int xdir = (x > 0 ? 1 : -1);
	int ydir = (y > 0 ? 1 : -1);

	int xstep = 0, ystep = 0;
	int xshift = 0, yshift = 0;
	int steps, shifts;

	if (std::abs(x) >= std::abs(y))
	{
		xstep = xdir;
		yshift = ydir;
		steps = std::abs(x);
		shifts = std::abs(y);
	}
	else
	{
		ystep = ydir;
		xshift = xdir;
		steps = std::abs(y);
		shifts = std::abs(x);
	}

	dot();

	int len=0;
	for (int i = 0; i < steps; ++i) {
		int new_x, new_y;
		len += shifts;
		if (len >= steps) {
			new_x = draw_x + xstep + xshift;
			new_y = draw_y + ystep + yshift;
			len -= steps;
		}
		else {
			new_x = draw_x + xstep;
			new_y = draw_y + ystep;
		}

		if (in_bounds(new_x, new_y)) {
			draw_x = new_x;
			draw_y = new_y;
			dot();
		}
		else {
			break;
		}
	}
}

void canvas::draw_text(const char *s, size_t len) {
	if (loaded_font == NULL) {
		if (draw_y - cascent() < 0 || draw_y + cdescent() >= buf->h)
			return;

		for (size_t i = 0; i < len; i++) {
			Uint8 char_code = s[i];
			
			draw_y -= cascent();
			for (int y = 0; y < charheight; y++) {
				for (int x = 0; x < charwidth; x++) {
					// check 'x'th bit
					if (charfaces[char_code][y] & (1 << x)) {
						pixel(draw_x, draw_y) = draw_color;
					}
					++draw_x;
					if (draw_x >= buf->w) return;
				}
				draw_x -= charwidth;
				++draw_y;
			}
			draw_x += charwidth;
			draw_y -= cdescent();
		}
	}
	else {
		SDL_Color text_color = {color_info.r, color_info.g, color_info.b, 0xFF};

		SDL_Surface* t;
		if (font_info.antialias) {
			t = TTF_RenderUTF8_Blended(loaded_font, s, text_color);
		} else {
			t = TTF_RenderUTF8_Solid(loaded_font, s, text_color);
		}

		SDL_Rect dest = { draw_x, draw_y - cascent(), 0, 0 };
		draw_x += t->w;
		SDL_BlitSurface(t, NULL, buf, &dest);
		SDL_FreeSurface(t);
	}
}

bool canvas::open(int width, int height) {
	if (buf) SDL_FreeSurface(buf);

	buf = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	draw_x = width/2;
	draw_y = height/2;
	
	return (buf != NULL);
}

bool canvas::save(const std::string& filename) const {
	return SDL_SaveBMP(buf, filename.c_str()) == 0;
}

void canvas::dot() {
	pixel(draw_x, draw_y) = draw_color;
}

int canvas::width() const { 
	return buf->w;
}

int canvas::height() const {
	return buf->h;
}

int canvas::cascent() const {
	return loaded_font == NULL
		? charheight - chardescent
		: TTF_FontAscent(loaded_font);
}

int canvas::cdescent() const {
	return loaded_font == NULL
		? chardescent
		: -TTF_FontDescent(loaded_font);
}

int canvas::twidth(const std::string &s) const {
	int width;
	if (loaded_font == NULL) {
		width = s.length() * charwidth;
	}
	else {
    TTF_SizeUTF8(loaded_font, s.c_str(), &width, NULL);
	}
	return width;	
}

canvas& canvas::operator=(const canvas &c) {
	draw_x = c.draw_x;
	draw_y = c.draw_y;
	draw_color = c.draw_color;
	transparent = c.transparent;

	if (c.buf) {
		buf = SDL_CreateRGBSurface(0, c.buf->w, c.buf->h, 32, 0, 0, 0, 0);
		SDL_BlitSurface(c.buf, NULL, buf, NULL);
	}
	else {
		buf = NULL;
	}

	return *this;
}

canvas& canvas::operator<<(font f) {
	font_info = f;
	if (loaded_font) TTF_CloseFont(loaded_font);
	loaded_font = TTF_OpenFont(f.name.c_str(), f.size);
	return *this;
}

canvas& canvas::operator<<(stamp s) {
	SDL_Surface *src_buf = s.c.buf;

	if (s.s_x == -1) s.s_x = 0;
	if (s.s_y == -1) s.s_y = 0;
	if (s.s_w == -1) s.s_w = src_buf->w;
	if (s.s_h == -1) s.s_h = src_buf->h;
	
	SDL_Rect src_rect = {s.s_x, s.s_y, s.s_w, s.s_h};
	SDL_Rect dst_rect = {s.d_x, s.d_y, 0, 0};

	// key out black pixels
	if (s.c.transparent)
		SDL_SetColorKey(src_buf, SDL_TRUE, 0);

	SDL_BlitSurface(src_buf, &src_rect, buf, &dst_rect);

	return *this;
}

canvas& canvas::operator<<(color c) {
	color_info = c;
	draw_color = (c.r << 16) | (c.g << 8) | c.b;
	return *this;
}

canvas& canvas::operator<<(const move &m) {
	int new_x = draw_x + m.diff_x;
	int new_y = draw_y + m.diff_y;

	if (in_bounds(new_x, new_y)) {
		draw_x = new_x;
		draw_y = new_y;
	}
	
	return *this;
}

canvas& canvas::operator<<(const move_to &mt) {
	if (in_bounds(mt.coord_x, mt.coord_y)) {
		draw_x = mt.coord_x;
		draw_y = mt.coord_y;
	}
	
	return *this;
}

canvas& canvas::operator<<(const line &l) {
	draw_line(l.diff_x, l.diff_y, true);
	return *this;
}

canvas& canvas::operator<<(const line_to &lt) {
	draw_line(lt.coord_x, lt.coord_y);
	return *this;
}

canvas& canvas::operator<<(const box &b) {
	SDL_Rect r = { draw_x, draw_y, b.w, b.h };
	SDL_FillRect(buf, &r, draw_color);
	return *this;
}

canvas& canvas::operator<<(const box_to &bt) {
	SDL_Rect r = { 
		draw_x, 
		draw_y,
		bt.bottomright_x - draw_x,
		bt.bottomright_y - draw_y 
	};
	SDL_FillRect(buf, &r, draw_color);
	return *this;
}

canvas& canvas::operator<<(const text &t) {
	draw_text(t.str.c_str(), t.str.length());
	return *this;
}

canvas& canvas::operator<<(const std::string &str) {
	draw_text(str.c_str(), str.length());
	return *this;
}

canvas& canvas::operator<<(const char *z_str) {
	draw_text(z_str, strlen(z_str));
	return *this;
}

// Groutput
groutput::groutput() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) exit(1);
	if (TTF_Init() < 0) exit(1);
}

groutput::~groutput() {
	if (buf) SDL_FreeSurface(buf);
	if (window) SDL_DestroyWindow(window);
	if (loaded_font) TTF_CloseFont(loaded_font);

	buf = NULL;
	window = NULL;
	loaded_font = NULL;

	SDL_Quit();
	TTF_Quit();
}

groutput& groutput::instance()
{
	static groutput instance;
	return instance;
}

bool groutput::open(int width, int height, std::string title, bool fullscreen) {
	Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

	window = SDL_CreateWindow(
		title.c_str(), 
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		width, height, flags);

	buf = SDL_GetWindowSurface(window);

	draw_x = width/2;
	draw_y = height/2;

	return (buf != NULL);
}

void groutput::refresh() {
	SDL_UpdateWindowSurface(window);
}

void groutput::show_mouse(bool shown) { 
	SDL_ShowCursor(shown);
}

void groutput::move_mouse(int x, int y) {
	SDL_WarpMouseInWindow(window, x, y);
}

// Grinput
grinput& grinput::instance()
{
	static grinput instance;
	return instance;
}

namespace 
{
SDL_TimerID timer_id = 0;

int find_key(int key) {
	pairptr begin = keysym_tbl;
	pairptr end = keysym_end;

	while (begin < end)
	{
		pairptr med = begin + (end-begin)/2;
		int mk = (*med)[0];

		if (mk == key) {
			return (*med)[1];
		} else if (mk < key) {
			begin = med+1;
		} else {
			end = med;
		}
	}

	return key;
}
}

grinput& genv::grinput::wait_event(event& ev)
{
	static event nullev = {0, 0, 0, 0, 0, 0};
	ev = nullev;
	if (quit)
		return *this;

	bool got = false;
	do
	{
		SDL_Event se;
		if (!SDL_WaitEvent(&se))
		{
			quit = true;
			return *this;
		}

		switch (se.type)
		{
		case SDL_QUIT:
			quit = true;
			got = true;
			break;
		
		case SDL_KEYUP:
		case SDL_KEYDOWN:
			ev.type = ev_key;
			ev.keycode = find_key(se.key.keysym.sym);
			if (se.type == SDL_KEYUP) ev.keycode *= -1;
			got = (ev.keycode != 0);
			break;
		
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			ev.type = ev_mouse;
			ev.button = se.button.button;
			if (se.button.state == SDL_RELEASED) ev.button *= -1;
			ev.pos_x = se.button.x;
			ev.pos_y = se.button.y;
			got = true;
			break;
		
		case SDL_MOUSEWHEEL:
			ev.type = ev_mouse;
			ev.button = (se.wheel.y > 0 ? btn_wheelup : btn_wheeldown);
			got = true;
			break;
		
		case SDL_MOUSEMOTION:
			ev.type = ev_mouse;
			ev.pos_x = se.motion.x;
			ev.pos_y = se.motion.y;
			got = true;
			break;

		case SDL_USEREVENT:
			ev.type = ev_timer;
			ev.time = se.user.code;
			got = true;
			break;

		default: 
			break;
		}
	} while (!got);

	return *this;
}

Uint32 fire_timer_event(Uint32 interval, void*)
{
	SDL_Event ev;
	ev.type = SDL_USEREVENT;
	ev.user.code = SDL_GetTicks();
	SDL_PushEvent(&ev);

	return interval;
}

void grinput::timer(int interval)
{
	if (timer_id)
	{
		SDL_RemoveTimer(timer_id);
		timer_id = 0;
	}

	if (interval > 0)
	{
		timer_id = SDL_AddTimer(interval, fire_timer_event, 0);
	}
}