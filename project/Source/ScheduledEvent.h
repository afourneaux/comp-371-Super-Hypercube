#pragma once

enum Event {
    GAME_START,
    GAME_END,
    EXIT_PROGRAM,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_SPACEBAR,
    INPUT_KEY_R,
    CREATE_SHAPE_AND_WALL,
    DESTROY_SHAPE_AND_WALL,
    DESTROY_SHAPE_AND_WALL_WORLD_TRANSITION,
    LEVEL_FAILED,
    LEVEL_SUCCESS,
    DISPLAY_SHAPE,
    PORTAL_ON_HORIZON,
    WORLD_TRANSITION,
    COLLISION_IMMINENT,
    UNLOCK_SHAPE_ROTATION,
    SHAPE_CREATED
};

struct ScheduledEvent {
    Event type;
    double timeRemaining;
};
