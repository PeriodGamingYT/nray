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

num vector3_length(
	struct vector3 a
) {
	return (num)sqrt(
		a.x * a.x +
		a.y * a.y +
		a.z * a.z
	);
}

#define DECL_VECTOR3_OPERATION(name, op) \
	struct vector3 name( \
		struct vector3 a, \
		struct vector3 b \
	) { \
		struct vector3 result = VECTOR3_INIT( \
			a.x op b.x, \
			a.y op b.y, \
			a.z op b.z \
		); \
	  \
		return result; \
	} \

DECL_VECTOR3_OPERATION(vector3_add, +)
DECL_VECTOR3_OPERATION(vector3_sub, -)
DECL_VECTOR3_OPERATION(vector3_mul, *)
DECL_VECTOR3_OPERATION(vector3_div, /)

//// scene
struct scene {
	struct sphere *spheres;
	int sphere_length;
	struct light *lights;
	int light_length;
};

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

//// lighting
enum light_type {
	POINT = 0,
	DIRECTIONAL,
	AMBIENT
};

struct light {
	enum light_type type;
	struct vector3 pos;
	num intensity;
};

num compute_lighting(
	struct vector3 pos,
	struct vector3 normal,
	struct scene *config
) {
	num intensity = 0;
	for(int i = 0; i < config->light_length; i++) {
		struct vector3 l = VECTOR3_INIT(0, 0, 0);
		switch(config->lights[i].type) {
			case AMBIENT:
				intensity += config->lights[i].intensity;
				continue;

			case DIRECTIONAL:
				l = config->lights[i].pos;
				break;

			case POINT:
				l = vector3_sub(config->lights[i].pos, pos);
				break;

			default:
				break;
		}

		num n_dot_l = vector3_dot(normal, l);
		if(n_dot_l > 0) {
			intensity += (
				config->lights[i].intensity *
				n_dot_l / (
					vector3_length(normal) *
					vector3_length(l)
				)
			);
		}
	}
	
	return intensity;
}

//// raycasting

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

	struct vector3 closest_t_vec = VECTOR3_INIT(
		closest_t, 
		closest_t, 
		closest_t
	);
	
	struct vector3 pos = vector3_mul(
		vector3_add(
			origin, 
			closest_t_vec
		), 
		
		d
	);
	
	struct vector3 normal = vector3_sub(pos, closest_sphere->center);
	num n_length = vector3_length(normal);
	struct vector3 n_length_vec = VECTOR3_INIT(
		n_length, 
		n_length, 
		n_length
	);
	
	normal = vector3_div(normal, n_length_vec);
	num computed = compute_lighting(
		pos, 
		normal,
		config
	);

	struct vector3 computed_vec = VECTOR3_INIT(
		computed,
		computed,
		computed
	);
	
	return vector3_mul(
		closest_sphere->color, 
		computed_vec
	);
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

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
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

	struct sphere config_spheres[] = {
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
		},

		{
			.color = VECTOR3_INIT(255, 255, 0),
			.center = VECTOR3_INIT(0, -5001, 0),
			.raduis = 5000
		}
	};

	struct light config_lights[] = {
		{
			.type = AMBIENT,
			.intensity = 0.2,
			.pos = VECTOR3_INIT(0, 0, 0)
		},

		{
			.type = POINT,
			.intensity = 0.6,
			.pos = VECTOR3_INIT(2, 1, 0)
		},

		{
			.type = DIRECTIONAL,
			.intensity = 0.2,
			.pos = VECTOR3_INIT(1, 4, 4)
		}
	};
	
	struct scene config = {
		.spheres = config_spheres,
		.sphere_length = ARRAY_SIZE(config_spheres),
		.lights = config_lights,
		.light_length = ARRAY_SIZE(config_lights)
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
