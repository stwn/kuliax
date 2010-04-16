/*
  Canonical Linux Ubuntu Usplash theme
  based on a design by Vadin Bu aka panoptus

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Oct. 04, 2009
  Esteban Torres <etorres2+public@gmail.com>
*/

#include <usplash-theme.h>
#include <usplash_backend.h>

//Palette indices
#define _BACKGROUND         0
#define _PROGRESSBACKGROUND 36
#define _PROGRESSFOREGROUND 214
#define _TEXTBACKGROUND     0
#define _TEXTFOREGROUND     241
#define _TEXTSUCCESS        254
#define _TEXTFAILURE        165

struct usplash_pixmap* pixmap_progress_back;
struct usplash_pixmap* pixmap_progress_fore;
void t_init(struct usplash_theme* theme);
void t_clear_progressbar(struct usplash_theme* theme);
void t_draw_progressbar(struct usplash_theme* theme, int percentage);
void t_animate_step(struct usplash_theme* theme, int pulsating);

extern struct usplash_font //Fonts
    font_7x13B,
    font_7x14B,
    font_9x15B;
struct usplash_theme // Themes
    usplash_theme_800_600,
    usplash_theme_1024_640,
    usplash_theme_1024_768,
    usplash_theme_1280_720,
    usplash_theme_1280_800,
    usplash_theme_1280_1024,
    usplash_theme_1440_900,
    usplash_theme_1600_1200,
    usplash_theme_1680_1050;
extern struct usplash_pixmap //Backgrounds
    pixmap_image_640_480,
    pixmap_image_800_600,
    pixmap_image_1024_640,
    pixmap_image_1024_768,
    pixmap_image_1280_720,
    pixmap_image_1280_800,
    pixmap_image_1280_1024,
    pixmap_image_1440_900,
    pixmap_image_1600_1200,
    pixmap_image_1680_1050;
extern struct usplash_pixmap //Progress bar backgrounds
    pixmap_progress_back_640_480,
    pixmap_progress_back_800_600,
    pixmap_progress_back_1024_640,
    pixmap_progress_back_1024_768,
    pixmap_progress_back_1280_720,
    pixmap_progress_back_1280_800,
    pixmap_progress_back_1280_1024,
    pixmap_progress_back_1440_900,
    pixmap_progress_back_1600_1200,
    pixmap_progress_back_1680_1050;
extern struct usplash_pixmap //Progress bar foregrounds
    pixmap_progress_fore_640_480,
    pixmap_progress_fore_800_600,
    pixmap_progress_fore_1024_640,
    pixmap_progress_fore_1024_768,
    pixmap_progress_fore_1280_720,
    pixmap_progress_fore_1280_800,
    pixmap_progress_fore_1280_1024,
    pixmap_progress_fore_1440_900,
    pixmap_progress_fore_1600_1200,
    pixmap_progress_fore_1680_1050;

struct usplash_theme usplash_theme = {
    .version = THEME_VERSION,
    .next = &usplash_theme_800_600,
    .ratio = USPLASH_4_3,
    
    // Background and font
    .pixmap = &pixmap_image_640_480,
    .font = &font_7x13B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 254,
    .progressbar_y      = 235,
    .progressbar_width  = 132,
    .progressbar_height = 18,

    // Text box position and size in pixels
    .text_x      = 65,
    .text_y      = 287,
    .text_width  = 511,
    .text_height = 95,

    // Text details
    .line_height  = 19,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 70,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_800_600 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1024_640,
    .ratio = USPLASH_4_3,
    
    // Background and font
    .pixmap = &pixmap_image_800_600,
    .font = &font_7x14B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 320,
    .progressbar_y      = 294,
    .progressbar_width  = 161,
    .progressbar_height = 22,

    // Text box position and size in pixels
    .text_x      = 145,
    .text_y      = 358,
    .text_width  = 511,
    .text_height = 100,

    // Text details
    .line_height  = 20,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 70,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1024_640 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1024_768,
    .ratio = USPLASH_16_9,
    
    // Background and font
    .pixmap = &pixmap_image_1024_640,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 428,
    .progressbar_y      = 313,
    .progressbar_width  = 169,
    .progressbar_height = 23,

    // Text box position and size in pixels
    .text_x      = 184,
    .text_y      = 381,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1024_768 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1280_720,
    .ratio = USPLASH_4_3,
    
    // Background and font
    .pixmap = &pixmap_image_1024_768,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 410,
    .progressbar_y      = 375,
    .progressbar_width  = 205,
    .progressbar_height = 28,

    // Text box position and size in pixels
    .text_x      = 184,
    .text_y      = 457,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1280_720 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1280_800,
    .ratio = USPLASH_16_9,
    
    // Background and font
    .pixmap = &pixmap_image_1280_720,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 545,
    .progressbar_y      = 352,
    .progressbar_width  = 191,
    .progressbar_height = 26,

    // Text box position and size in pixels
    .text_x      = 312,
    .text_y      = 428,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1280_800 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1280_1024,
    .ratio = USPLASH_16_9,
    
    // Background and font
    .pixmap = &pixmap_image_1280_800,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 534,
    .progressbar_y      = 392,
    .progressbar_width  = 213,
    .progressbar_height = 29,

    // Text box position and size in pixels
    .text_x      = 312,
    .text_y      = 477,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1280_1024 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1440_900,
    .ratio = USPLASH_4_3,
    
    // Background and font
    .pixmap = &pixmap_image_1280_1024,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 501,
    .progressbar_y      = 501,
    .progressbar_width  = 279,
    .progressbar_height = 38,

    // Text box position and size in pixels
    .text_x      = 312,
    .text_y      = 611,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1440_900 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1600_1200,
    .ratio = USPLASH_16_9,
    
    // Background and font
    .pixmap = &pixmap_image_1440_900,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 599,
    .progressbar_y      = 441,
    .progressbar_width  = 242,
    .progressbar_height = 33,

    // Text box position and size in pixels
    .text_x      = 392,
    .text_y      = 537,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1600_1200 = {
    .version = THEME_VERSION,
    .next = &usplash_theme_1680_1050,
    .ratio = USPLASH_4_3,
    
    // Background and font
    .pixmap = &pixmap_image_1600_1200,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 639,
    .progressbar_y      = 588,
    .progressbar_width  = 323,
    .progressbar_height = 44,

    // Text box position and size in pixels
    .text_x      = 472,
    .text_y      = 716,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};
struct usplash_theme usplash_theme_1680_1050 = {
    .version = THEME_VERSION,
    .next = NULL,
    .ratio = USPLASH_16_9,
    
    // Background and font
    .pixmap = &pixmap_image_1680_1050,
    .font = &font_9x15B,
    
    // Palette indices
    .background             = _BACKGROUND,
    .progressbar_background = _PROGRESSBACKGROUND,
    .progressbar_foreground = _PROGRESSFOREGROUND,
    .text_background        = _TEXTBACKGROUND,
    .text_foreground        = _TEXTFOREGROUND,
    .text_success           = _TEXTSUCCESS,
    .text_failure           = _TEXTFAILURE,

    // Progress bar position and size in pixels
    .progressbar_x      = 697,
    .progressbar_y      = 515,
    .progressbar_width  = 286,
    .progressbar_height = 39,

    // Text box position and size in pixels
    .text_x      = 512,
    .text_y      = 628,
    .text_width  = 657,
    .text_height = 105,

    // Text details
    .line_height  = 21,  //Line Height in pixels
    .line_length  = 50,  //Line Length in characters
    .status_width = 90,  //Status Width in pixels

    // Functions
    .init = t_init,
    .clear_progressbar = t_clear_progressbar,
    .draw_progressbar = t_draw_progressbar,
    .animate_step = t_animate_step,
};


int num_steps=0;
void t_init(struct usplash_theme *theme)
{
    int x, y;
    usplash_getdimensions(&x, &y);
    theme->progressbar_x = (x - theme->pixmap->width)/2 + theme->progressbar_x;
    theme->progressbar_y = (y - theme->pixmap->height)/2 + theme->progressbar_y;
    
    //Choose the appropriate progress bars
    switch( x )
    {
        case 1680:
            if(y==1050)
            {
                pixmap_progress_back = &pixmap_progress_back_1680_1050;
                pixmap_progress_fore = &pixmap_progress_fore_1680_1050;
            }
            break;
        case 1600:
            if(y==1200)
            {
                pixmap_progress_back = &pixmap_progress_back_1600_1200;
                pixmap_progress_fore = &pixmap_progress_fore_1600_1200;
            }
            break;
        case 1440:
            if(y==900)
            {
                pixmap_progress_back = &pixmap_progress_back_1440_900;
                pixmap_progress_fore = &pixmap_progress_fore_1440_900;
            }
            break;
        case 1280:
            if(y==1024)
            {
                pixmap_progress_back = &pixmap_progress_back_1280_1024;
                pixmap_progress_fore = &pixmap_progress_fore_1280_1024;
            }
            else if(y==800)
            {
                pixmap_progress_back = &pixmap_progress_back_1280_800;
                pixmap_progress_fore = &pixmap_progress_fore_1280_800;
            }
            else if(y==720)
            {
                pixmap_progress_back = &pixmap_progress_back_1280_720;
                pixmap_progress_fore = &pixmap_progress_fore_1280_720;
            }
            break;
        case 1024:
            if(y==768)
            {
                pixmap_progress_back = &pixmap_progress_back_1024_768;
                pixmap_progress_fore = &pixmap_progress_fore_1024_768;
            }
            else if(y==640)
            {
                pixmap_progress_back = &pixmap_progress_back_1024_640;
                pixmap_progress_fore = &pixmap_progress_fore_1024_640;
            }
            break;
        case 800:
            if(y==600)
            {
                pixmap_progress_back = &pixmap_progress_back_800_600;
                pixmap_progress_fore = &pixmap_progress_fore_800_600;
            }
            break;
        case 640:
            if(y==480)
            {
                pixmap_progress_back = &pixmap_progress_back_640_480;
                pixmap_progress_fore = &pixmap_progress_fore_640_480;
            }
            break;
        default:
            pixmap_progress_back = &pixmap_progress_back_800_600;
            pixmap_progress_fore = &pixmap_progress_fore_800_600;
    }
    num_steps = (pixmap_progress_back->width - 28)/2 - 1;
}

void t_clear_progressbar(struct usplash_theme *theme)
{
    t_draw_progressbar(theme, 0);
}

void t_draw_progressbar(struct usplash_theme *theme, int percentage)
{
    int w = (pixmap_progress_back->width * percentage / 100);

    usplash_put(theme->progressbar_x, theme->progressbar_y, pixmap_progress_back);

    if(percentage == 0)
        return;
    
    if(percentage < 0)
        usplash_put_part(theme->progressbar_x - w, theme->progressbar_y,
                         pixmap_progress_back->width + w, pixmap_progress_back->height,
                         pixmap_progress_fore,
                         -w, 0
                         );
    else
        usplash_put_part(theme->progressbar_x, theme->progressbar_y,
                         w, pixmap_progress_back->height, 
                         pixmap_progress_fore,
                         0, 0
                         );
}

void t_animate_step(struct usplash_theme* theme, int pulsating)
{
    static int pulsate_step = 0;
    static int pulse_width = 28;
    static int step_width = 2;
    int x1;

    if (pulsating) {
        t_draw_progressbar(theme, 0);
    
        if(pulsate_step < num_steps/2+1)
            x1 = 2 * step_width * pulsate_step;
        else
            x1 = pixmap_progress_back->width - pulse_width - 2 * step_width * (pulsate_step - num_steps/2 + 1);

        usplash_put_part(theme->progressbar_x + x1, theme->progressbar_y,
                         pulse_width, pixmap_progress_fore->height,
                         pixmap_progress_fore,
                         x1, 0
                         );

        pulsate_step = (pulsate_step + 1) % num_steps;
    }
}
