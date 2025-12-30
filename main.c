#include <math.h>
#include <stdio.h>
#include <raylib.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define WIDTH 1000
#define HEIGHT 1000

#define BIRD_COUNT 30

#define BIRD_RADIUS 2
#define TARGET_FPS 60
#define SPAWN_BORDER 100
#define MIN_SPEED -300
#define MAX_SPEED -MIN_SPEED
#define COLLISION_IMMEDIATE_OFFSET 10
#define COHESION_WEIGHT 0.2
#define ALIGNMENT_WEIGHT 0.8

#define EPSILON 0.1
#define K_VERTICAL 3.0/ HEIGHT
#define K_HORIZONTAL 3.0 / WIDTH
#define SQUARE(x) ((x) * (x))

struct Bird {
  float x;
  float y;
  float x_velocity;
  float y_velocity;
};

struct Bird **birds;
Camera2D camera = {0};

void drawBird(struct Bird* bird) {
  DrawCircle(bird->x, bird->y, BIRD_RADIUS, (Color){100, 100, 100, 100});
  DrawLine(bird->x, bird->y, bird->x+bird->x_velocity, bird->y+bird->y_velocity, (Color){255, 0, 0, 255});
}

/* void applyDirectionCorrection(struct Bird *bird) { */
/*   float x_direction = 0.0; */
/*   float y_direction = 0.0; */
/*     for (int i = 0; i < BIRD_COUNT; i++) { */
/*       x_direction += birds[i]->x_velocity*exp(K_HORIZONTAL*fabs(bird->x - birds[i]->x)); */
/*       y_direction += birds[i]->y_velocity*exp(K_VERTICAL*fabs(bird->y - birds[i]->y)); */
/*     } */
/*     float current_direction = y_direction/x_direction; */
/*   float this_bird_speed = sqrtf(bird->y_velocity * bird->y_velocity + */
/*                                 bird->x_velocity * bird->x_velocity); */
/*   bird->y_velocity = this_bird_speed / (sqrtf((1 + current_direction*current_direction)/(current_direction*current_direction))); */
/*   bird->x_velocity = bird->y_velocity / current_direction; */
  
/*   // bird->y_velocity = this_bird_speed / (sqrtf(1+current_direction*current_direction)/(current_direction*current_direction)); */
/* } */

/* void applyDirectionCorrection(struct Bird *bird) { */
/*     float x_direction = 0.0f;  // Initialize! */
/*     float y_direction = 0.0f; */
    
/*     for (int i = 0; i < BIRD_COUNT; i++) { */
/*         if (birds[i] == bird) continue;  // Skip self */
        
/*         float dx = fabs(bird->x - birds[i]->x); */
/*         float dy = fabs(bird->y - birds[i]->y); */
        
/*         // Use negative exponent for decay with distance */
/*         float weight_x = exp(-K_HORIZONTAL * dx); */
/*         float weight_y = exp(-K_VERTICAL * dy); */
        
/*         x_direction += birds[i]->x_velocity * weight_x; */
/*         y_direction += birds[i]->y_velocity * weight_y; */
/*     } */
    
/*     // Check for zero direction */
/*     if (fabs(x_direction) < 1e-6f && fabs(y_direction) < 1e-6f) { */
/*         return;  // No correction needed */
/*     } */
    
/*     // Normalize the direction vector */
/*     float direction_magnitude = sqrtf(x_direction * x_direction + y_direction * y_direction); */
/*     x_direction /= direction_magnitude; */
/*     y_direction /= direction_magnitude; */
    
/*     // Apply direction while preserving speed */
/*     float this_bird_speed = sqrtf(bird->x_velocity * bird->x_velocity +  */
/*                                    bird->y_velocity * bird->y_velocity); */
/*     bird->x_velocity = x_direction * this_bird_speed; */
/*     bird->y_velocity = y_direction * this_bird_speed; */
/* } */


void applyDirectionCorrection(struct Bird *bird) {
    float x_position = 0.0f;  // Initialize!
    float y_position = 0.0f;

    float x_direction = 0.0f;
    float y_direction = 0.0f;
    
    float total_weight_x = 0.0f;
    float total_weight_y = 0.0f;

    for (int i = 0; i < BIRD_COUNT; i++) {
      if (birds[i] == bird)
        continue;

      float dx = fabs(bird->x - birds[i]->x);
      float dy = fabs(bird->y - birds[i]->y);

      float weight_x = exp(-K_HORIZONTAL * dx);
      float weight_y = exp(-K_VERTICAL * dy);

      
      x_position += birds[i]->x * weight_x;
      y_position += birds[i]->y * weight_y;

      x_direction += birds[i]->x_velocity * weight_x;
      y_direction += birds[i]->y_velocity * weight_y;
      
      total_weight_x += weight_x;
      total_weight_y += weight_y;
    }
    
    // Normalize by total weight to get weighted average position
    x_position /= total_weight_x;
    y_position /= total_weight_y;
    x_direction /= total_weight_x;
    y_direction /= total_weight_y;
    
    float x_cohesion = x_position - bird->x;
    float y_cohesion = y_position - bird->y;
    // Check for zero direction
    if (fabs(x_direction) < 1e-6f && fabs(y_direction) < 1e-6f) {
      return; // No correction needed
    }
    if (fabs(x_cohesion) < 1e-6f && fabs(y_cohesion) < 1e-6f) {
      return; // No correction needed
    }
    // Normalize the direction vector
    float direction_magnitude = sqrtf(x_direction * x_direction + y_direction * y_direction);
    x_direction /= direction_magnitude;
    y_direction /= direction_magnitude;

    float cohesion_magnitude = sqrtf(SQUARE(x_cohesion) + SQUARE(y_cohesion));
    x_cohesion /= cohesion_magnitude;
    y_cohesion /= cohesion_magnitude;
    
    // Apply direction while preserving speed
    float this_bird_speed = sqrtf(bird->x_velocity * bird->x_velocity + 
                                   bird->y_velocity * bird->y_velocity);
    // Combine them (with optional weights):
    float x_combined = COHESION_WEIGHT * x_cohesion + ALIGNMENT_WEIGHT * x_direction;
    float y_combined = COHESION_WEIGHT * y_cohesion + ALIGNMENT_WEIGHT * y_direction;

    // Renormalize the combined vector:
    float magnitude = sqrtf(x_combined * x_combined + y_combined * y_combined);
    float x_final = x_combined / magnitude;
    float y_final = y_combined / magnitude;

    // Then apply speed:
    bird->x_velocity = x_final * this_bird_speed;
    bird->y_velocity = y_final * this_bird_speed;
}

void update() {
  BeginDrawing();
  BeginMode2D(camera);
  ClearBackground(RAYWHITE);
  for (int i = 0; i < BIRD_COUNT; i++) {
    // Draw each bird and its velocity vector to the screen
    drawBird(birds[i]);
    // Calculate positions for the next frame
    
    float modifier = 1.0/TARGET_FPS;
    if (GetFPS() > 0) {
      modifier = 1.0/GetFPS();
    }
    DrawFPS(20, 20);

    applyDirectionCorrection(birds[i]);
    
    // Update position first
    birds[i]->x += birds[i]->x_velocity * modifier;
    birds[i]->y += birds[i]->y_velocity * modifier;

    // Wrap around boundaries
    if (birds[i]->x < 0) {
      birds[i]->x += WIDTH;
    }
    if (birds[i]->x > WIDTH) {
      birds[i]->x -= WIDTH;
    }
    if (birds[i]->y < 0) {
      birds[i]->y += HEIGHT;
    }
    if (birds[i]->y > HEIGHT) {
      birds[i]->y -= HEIGHT;
    }
    printf("The bird's x, y, is now %f, %f", birds[i]->x, birds[i]->y);
  }
  // WaitTime(1);
  EndMode2D();
  EndDrawing();
}

void setup() {
  SetRandomSeed(GetTime());
  birds = malloc(sizeof(struct Bird*)*BIRD_COUNT);
  for (int i = 0; i < BIRD_COUNT; i++) {
    birds[i] = malloc(sizeof(struct Bird));
    birds[i]->x = GetRandomValue(SPAWN_BORDER, WIDTH - SPAWN_BORDER);
    birds[i]->y = GetRandomValue(SPAWN_BORDER, HEIGHT - SPAWN_BORDER);
    birds[i]->x_velocity = GetRandomValue(MIN_SPEED, MAX_SPEED);
    birds[i]->y_velocity = GetRandomValue(MIN_SPEED, MAX_SPEED);
  }
   
  SetTargetFPS(TARGET_FPS);
  

  InitWindow(WIDTH, HEIGHT, "Birrds");
  InitAudioDevice();

  camera.target = (Vector2){ WIDTH/2.0f, HEIGHT/2.0f };
  camera.offset = (Vector2){ WIDTH/2.0f, HEIGHT/2.0f };
  camera.rotation = 0.0f;
   camera.zoom = 1.0f;   
  if (!IsAudioDeviceReady()) {
    printf("Audio Device Setup Failed:(\n");
  }
  return;
}

int main() {
  printf("Hello, workd\n");
  setup();
  while (!WindowShouldClose()) {
    update();
  }
  return 0;
}
