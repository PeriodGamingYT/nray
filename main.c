//// includes
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>

//// declarations
#define RAYCAST_MAX 1000
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define VIEWPORT_WIDTH 1
#define VIEWPORT_HEIGHT 1
#define PROJECTION_PLANE_D 1
typedef long double num;

//// vectors
struct vector3 {
	num x;
	num y;
	num z;
};

#define VECTOR3_INIT(arg_x, arg_y, arg_z) \
	{ \
		.x = arg_x, \
		.y = arg_y, \
		.z = arg_z \
	}

num vector3_dot(
	struct vector3 a,
	struct vector3 b
) {
	return (
		a.x * b.x +
		a.y * b.y +
		a.z * b.z
	);
}

struct vector3 vector3_sub(
	struct vector3 a,
	struct vector3 b
) {
	struct vector3 result = VECTOR3_INIT(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	);

	return result;
}

//// spheres
struct sphere {
	struct vector3 color;
	struct vector3 center;
	num raduis;
};

num *intersect_ray_sphere(
	struct vector3 origin,
	struct vector3 d,
	struct sphere arg_sphere
) {
	num t[2] = { 0 };
	num inf_t[2] = { RAYCAST_MAX };
	num r = arg_sphere.raduis;
	struct vector3 co = vector3_sub(
		origin, 
		arg_sphere.center
	);

	num a = vector3_dot(d, d);
	num b = 2 * vector3_dot(co, d);
	num c = vector3_dot(co, co) - r * r;
	num discriminant = (b * b) - (4 * a * c);
	if(discriminant < 0) {
		return inf_t;
	}

	t[0] = (-b + (num)sqrt(discriminant)) / (2 * a);
	t[1] = (-b - (num)sqrt(discriminant)) / (2 * a);
	return t;
}

//// raycasting
struct scene {
	struct sphere *spheres;
	int sphere_length;
};

#define X_IN(x, a, b) (((x) >= (a)) && ((x) <= (b)))
struct vector3 trace_ray(
	struct vector3 origin,
	struct vector3 d,
	num t_min,
	num t_max,
	struct scene *config
) {
	num closest_t = RAYCAST_MAX;
	struct sphere *closest_sphere = NULL;
	for(int i = 0; i < config->sphere_length; i++) {
		num *t = intersect_ray_sphere(
			origin,
			d,
			config->spheres[i]
		);

		if(X_IN(t[0], t_min, t_max) && t[0] < closest_t) {
			closest_t = t[0];
			closest_sphere = &config->spheres[i];
		}

		if(X_IN(t[1], t_min, t_max) && t[1] < closest_t) {
			closest_t = t[1];
			closest_sphere = &config->spheres[i];
		}
	}
	
	if(closest_sphere == NULL) {
		struct vector3 background_color = VECTOR3_INIT(0, 0, 0);
		return background_color;
	}

	return closest_sphere->color;
}

struct vector3 screen_to_viewport(
	num x,
	num y
) {
	struct vector3 result = VECTOR3_INIT(
		x * VIEWPORT_WIDTH / SCREEN_WIDTH,
		y * VIEWPORT_HEIGHT / SCREEN_HEIGHT,
		PROJECTION_PLANE_D	
	);

	return result;
}

void raycast_with_renderer(
	struct scene *config,
	SDL_Renderer *renderer
) {
	struct vector3 origin = VECTOR3_INIT(0, 0, 0);
	for(
		num x = -SCREEN_WIDTH / 2; 
		x < SCREEN_WIDTH / 2; 
		x++
	) {
		for(
			num y = -SCREEN_HEIGHT / 2;
			y < SCREEN_HEIGHT / 2;
			y++
		) {
			struct vector3 d = screen_to_viewport(x, y);
			struct vector3 color = trace_ray(
				origin,
				d,
				1,
				RAYCAST_MAX,
				config
			);
			
			SDL_SetRenderDrawColor(
				renderer,
				(int)color.x,
				(int)color.y,
				(int)color.z,
				255
			);

			SDL_RenderDrawPoint(
				renderer, 
				(int)(x + (SCREEN_WIDTH / 2)), 

				// With x and y correction, y is upside
				// down so I need to correct for that.
				(int)(SCREEN_HEIGHT - (y + (SCREEN_HEIGHT / 2))) 
			);
		}
	}

	SDL_RenderPresent(renderer);
}

//// main
void die(char *function) {
	fprintf(stderr, "%s(): %s\n", function, SDL_GetError());
	exit(-1);
}

static SDL_Window *window;
static SDL_Renderer *renderer;
void quit_sdl() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

#define SPHERE_LENGTH 3
int main() {
	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		die("SDL_Init");
	}

	atexit(quit_sdl);
	SDL_CreateWindowAndRenderer(
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0,
		&window,
		&renderer
	);

	if(window == NULL || renderer == NULL) {
		die("SDL_CreateWindowAndRenderer");
	}

	SDL_Event event;

	struct sphere config_spheres[SPHERE_LENGTH] = {
		{
			.color = VECTOR3_INIT(255, 0, 0),
			.center = VECTOR3_INIT(0, -1, 3),
			.raduis = 1
		},

		{
			.color = VECTOR3_INIT(0, 0, 255),
			.center	= VECTOR3_INIT(2, 0, 4),
			.raduis = 1
		},

		{
			.color = VECTOR3_INIT(0, 255, 0),
			.center = VECTOR3_INIT(-2, 0, 4),
			.raduis = 1
		}
	};
	
	struct scene config = {
		.spheres = config_spheres,
		.sphere_length = SPHERE_LENGTH
	};

	raycast_with_renderer(&config, renderer);
	int is_keep_open = 1;
	while(
		SDL_PollEvent(&event) != -1 && 
		is_keep_open
	) {
		switch(event.type) {
			case SDL_QUIT:
				is_keep_open = 0;
				break;

			default:
				break;
		}
	}
	
	exit(0);
}
