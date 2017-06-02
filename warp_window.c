
#define LENGTH_OF_WRIST_TWIST_MODEL (10)
#define LENGTH_OF_ARM_EXTEND_MODEL  (10)
#define LENGTH_OF_DOUBLE_CLAP_MODEL (15)

#define MAX_WRIST_TWIST_CYCLES  (10)
#define MIN_WRIST_TWIST_CYCLES  (3)

#define MAX_ARM_EXTEND_CYCLES   (10)
#define MIN_ARM_EXTEND_CYCLES   (5)

#define MAX_DOUBLE_CLAP_CYCLES  (15)
#define MIN_DOUBLE_CLAP_CYCLES  (5)

#define CYCLESET_HISTORY_LEN    (20)


// when you get a new timeset, you look at the last MIN_CYCLES timesets and load them into the DTW matrix
// if no gesture is found 
// hold the last 2 seconds in memory

/* Define as NUM_ACCEL_AXES for now, but could add more like pitch and chaos */
#define NUM_DTW_AXES    (NUM_ACCEL_AXES)

typedef struct 
{

    const uint8       num_axes;
    const uint16      dtw_max_succes_score;

    uint8             cycles_since_recognized;

    axis_point        **model;
    const uint8       warp_model_len;

    axis_point        **trial;
    const uint8       min_cycles;
    const uint8       max_cycles;

} accel_warp;


// Need to decide how the history buffers will be implemented:
// if a circular buffer then the DTW algorithm needs to be modified to wraparound.
// if a chronological buffer then each value needs to be copied down the line each time a new one comes in.
//      With only 20 * 3 = 60 values in each buffer, this seems like it might be fast enough.
//      Otherwise, we will have to modify DTW to handle a circular buffer

static axis_point[CYCLESET_HISTORY_LEN][NUM_DTW_AXES] left_accel_history_buf;
static axis_point[CYCLESET_HISTORY_LEN][NUM_DTW_AXES] right_accel_history_buf;
static axis_point[CYCLESET_HISTORY_LEN][NUM_DTW_AXES] main_accel_history_buf;

// when a new cycleset comes in, add the two wrists data points to this buffer
static axis_point[CYCLESET_HISTORY_LEN][NUM_DTW_AXES*2] both_wrist_accel_history_buf;

/*Private function prototypes */

static bool sweepWarpWindow(accel_warp *warp);
// TODO: add this in
static uint16 runDTW(uint16* model, uint16* trial, uint16 model_len, uint16 trial_len, uint8 num_axes);

static bool sweepWarpWindow(accel_warp *warp)
{
    uint8 i;
    uint16 score;

    if (warp->cycles_since_recognized < warp->max_cycles)
    {
        warp->cycles_since_recognized++;
    }

    for (i = warp->min_cycles; i <= warp_cycles_since_recognized; i++)
    {
        uint16 buf_offset = sizeof(axis_point) * warp->num_axes * (CYCLESET_HISTORY_LEN - i);
        score = runDTW(&(warp->model[0]), &(warp->trial[0])+buf_offset, warp->warp_model_len, i, warp->num_axes);
        if (score <= warp->dtw_max_succes_score)
        {
            warp->cycles_since_recognized = 0;
            return TRUE;
        }
    }
    return FALSE;
}

