#include "input_dev.h"
#include "logic.h"
#include "message.h"
#include "queue.h"
#include "dev_iio.h"
#include "platform.h"

#include <stdlib.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <dirent.h>

static const char *input_path = "/dev/input/";
static const char *iio_path = "/sys/bus/iio/devices/";
static const char *hidraw_path = "/dev/hidraw1";

uint32_t input_filter_imu_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags) {
/*
    int32_t gyro_x = 0, gyro_y = 0, gyro_z = 0, accel_x = 0, accel_y = 0, accel_z = 0;
    if (gyroscope_mouse_translation > 0) {
        for (uint32_t i = 0; i < *count; ++i) {
            if (events[i].type != EV_ABS) {
                continue;
            }

            if (events[i].code == ABS_X) {
                accel_x = events[i].value;
            } else if (events[i].code == ABS_Y) {
                accel_y = events[i].value;
            } else if (events[i].code == ABS_Z) {
                accel_z = events[i].value;
            } else if (events[i].code == ABS_RX) {
                gyro_x = events[i].value;
            } else if (events[i].code == ABS_RY) {
                gyro_y = events[i].value;
            } else if (events[i].code == ABS_RZ) {
                gyro_z = events[i].value;
            }
        }

        uint32_t w = 0;

        if (gyro_x != 0) {
            events[w].type = EV_REL;
            events[w].code = REL_Y;
            events[w].value = (float)gyro_x * -1.0;
            ++w;
        }

        if (gyro_y != 0) {
            events[w].type = EV_REL;
            events[w].code = REL_X;
            events[w].value = (float)gyro_y * +1.0;
            ++w;
        }

        *count = w;

        flags |= EV_MESSAGE_FLAGS_PRESERVE_TIME | INPUT_FILTER_FLAGS_MOUSE; 
    }
*/
    return INPUT_FILTER_FLAGS_DO_NOT_EMIT;
}

uint32_t input_filter_identity(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags) {
    return INPUT_FILTER_FLAGS_NONE;
}

uint32_t input_filter_asus_kb(struct input_event* events, size_t* size, uint32_t* count, uint32_t* flags) {
    

    return INPUT_FILTER_FLAGS_NONE;
}

static struct libevdev* ev_matches(const char* sysfs_entry, const uinput_filters_t* const filters) {
    struct libevdev *dev = NULL;

    int fd = open(sysfs_entry, O_RDWR);
    if (fd < 0) {
        //fprintf(stderr, "Cannot open %s, device skipped.\n", sysfs_entry);
        return NULL;
    }

    if (libevdev_new_from_fd(fd, &dev) != 0) {
        //fprintf(stderr, "Cannot initialize libevdev from this device (%s): skipping.\n", sysfs_entry);
        close(fd);
        return NULL;
    }

    const char* name = libevdev_get_name(dev);
    if ((name != NULL) && (strcmp(name, filters->name) != 0)) {
        //fprintf(stderr, "The device name (%s) for device %s does not matches the expected one %s.\n", name, sysfs_entry, filters->name);
        libevdev_free(dev);
        close(fd);
        return NULL;
    }

    const int grab_res = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (grab_res != 0) {
        fprintf(stderr, "Unable to grab the device (%s): %d.\n", sysfs_entry, grab_res);
        //libevdev_free(dev);
        //close(fd);
        return dev;
    }

    return dev;
}

static dev_iio_t* iio_matches(const char* sysfs_entry, const iio_filters_t* const filters) {
    dev_iio_t *const dev_iio = dev_iio_create(sysfs_entry);
    if (dev_iio == NULL) {
        fprintf(stderr, "Could not create iio device.\n");
        return NULL;
    }

    const char* const iio_name = dev_iio_get_name(dev_iio);
    if (abs(strcmp(iio_name, filters->name)) != 0) {
        fprintf(stderr, "Error: iio device name does not match, expected %s got %s.\n", filters->name, iio_name);
        dev_iio_destroy(dev_iio);
        return NULL;
    }

    return dev_iio;
}

static pthread_mutex_t input_acquire_mutex = PTHREAD_MUTEX_INITIALIZER;

static char* open_sysfs[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
};

#define MAX_MESSAGES_IN_FLIGHT 32
#define DEFAULT_EVENTS_IN_REPORT 8

#define INPUT_CTX_FLAGS_READ_TERMINATED 0x00000001U

struct input_ctx {
    struct libevdev* dev;
dev_iio_t *iio_dev;
queue_t* queue;
    queue_t* rumble_queue;
    controller_settings_t* settings;
    uint32_t flags;
    message_t messages[MAX_MESSAGES_IN_FLIGHT];
    ev_input_filter_t input_filter_fn;
};

static void* iio_read_thread_func(void* ptr) {
    struct input_ctx* ctx = (struct input_ctx*)ptr;

    message_t* msg = NULL;
    
    int rc = -1;

    do {
        if (msg == NULL) {
            for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
                if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE) != 0) {
                    msg = &ctx->messages[h];
                    //TODO: msg->ev_count = 0;
                    break;
                }
            }
        }

        if (msg == NULL) {
            fprintf(stderr, "iio: Events are stalled.\n");
            continue;
        }

        rc = dev_iio_read_imu(ctx->iio_dev, &msg->data.imu);
        if (rc == 0) {
            // OK: good read. go on....
        } else if (rc == -ENOMEM) {
            fprintf(stderr, "Error: out-of-memory will skip the current frame.\n");
            continue;
        } else {
            fprintf(stderr, "Error: reading %s: %d\n", dev_iio_get_name(ctx->iio_dev), rc);
            break;
        }

        // clear out flags
        msg->flags = 0x00000000U;

        if (queue_push(ctx->queue, (void*)msg) != 0) {
            fprintf(stderr, "Error pushing iio event.\n");

            // flag the memory to be safe to reuse
            msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
        }

        // TODO: configure equal as sampling rate
        // usleep(1250);
        usleep(15000);


        // either way.... fill a new buffer on the next cycle
        msg = NULL;
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    return NULL;
}

static void* input_read_thread_func(void* ptr) {
    struct input_ctx* ctx = (struct input_ctx*)ptr;
    struct libevdev* dev = ctx->dev;

    int has_syn = libevdev_has_event_type(ctx->dev, EV_SYN);

    int rc = 1;

    message_t* msg = NULL;
    
    do {
        
        if (msg == NULL) {
            for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
                if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE) != 0) {
                    msg = &ctx->messages[h];
                    msg->data.event.ev_count = 0;
                    break;
                }
            }
        }

        if (msg == NULL) {
            fprintf(stderr, "udev: Events are stalled.\n");
            continue;
        }

        struct input_event read_ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &read_ev);
        if (rc == 0) {
            const int is_syn = (read_ev.type == EV_SYN) && (read_ev.code == SYN_REPORT);
            
            if (read_ev.type == EV_MSC) {
                if (read_ev.code == MSC_SCAN) {
#if defined(IGNORE_INPUT_SCAN)
                    continue;
#endif // IGNORE_INPUT_SCAN
                } else if (read_ev.code == MSC_TIMESTAMP) {
                    // the output device will handle that
                    //printf("MSC_TIMESTAMP found. Ignoring...\n");
                }
            }

            if ((!has_syn) || ((has_syn) && (!is_syn))) {
#if defined(INCLUDE_INPUT_DEBUG)
                printf(
                    "Input: %s %s %d\n",
                    libevdev_event_type_get_name(read_ev.type),
                    libevdev_event_code_get_name(read_ev.type, read_ev.code),
                    read_ev.value
                );
#endif

                if ((msg->data.event.ev_count+1) == msg->data.event.ev_size) {
                    printf("maximum number of events reached, buffer enlarged.\n");
                    
                    const size_t new_size = msg->data.event.ev_size * 2;
                    struct input_event* new_buf = malloc(sizeof(struct input_event) * new_size);
                    if (new_buf != NULL) {
                        void* old_buf = (void*)msg->data.event.ev;
                        
                        // copy events already in the buffer
                        memcpy((void*)new_buf, (const void*)old_buf, sizeof(struct input_event) * msg->data.event.ev_size);
                        
                        // copy the new event
                        memcpy((void*)(&new_buf[msg->data.event.ev_count]), (const void*)&read_ev, sizeof(struct input_event));
                        ++msg->data.event.ev_count;

                        msg->data.event.ev = new_buf;
                        msg->data.event.ev_size = new_size;

                        free(old_buf);
                    } else {
                        fprintf(stderr, "Unable to allocate data for incoming events.");
                    }
                } else {
                    // just copy the input event
                    msg->data.event.ev[msg->data.event.ev_count] = read_ev;
                    ++msg->data.event.ev_count;
                }
                
            }

            if ((!has_syn) || ((has_syn) && (is_syn))) {
#if defined(INCLUDE_INPUT_DEBUG)
                printf("Sync ---------------------------------------\n");
#endif

                // clear out flags
                msg->flags = 0x00000000U;
                msg->data.event.ev_flags = 0x00000000U;

                const uint32_t input_filter_res = ctx->input_filter_fn(msg->data.event.ev, &msg->data.event.ev_size, &msg->data.event.ev_count, &msg->data.event.ev_flags);

                if (((input_filter_res & INPUT_FILTER_FLAGS_DO_NOT_EMIT) == 0) && (msg->data.event.ev_count > 0)) {
                    if (queue_push(ctx->queue, (void*)msg) != 0) {
                        fprintf(stderr, "Error pushing event.\n");

                        // flag the memory to be safe to reuse
                        msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
                    }
                } else {
                    // flag the memory to be safe to reuse
                    msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
                }
                // either way.... fill a new buffer on the next cycle
                msg = NULL;
            }
        }
    } while (rc == 1 || rc == 0 || rc == -EAGAIN);

    ctx->flags |= INPUT_CTX_FLAGS_READ_TERMINATED;

    return NULL;
}

#define HIDRAW_BUFFER_SIZE 64
typedef struct {
    unsigned char buffer[HIDRAW_BUFFER_SIZE];
    ssize_t bytes_read;
    pthread_mutex_t mutex;
    volatile int termination_condition;  // Make it volatile if changed by another thread
} hidraw_buffer_t;
// Define the pattern to match
const unsigned char pattern[] = {0x19, 0x04, 0x01, 0x74};
const size_t pattern_size = sizeof(pattern) / sizeof(pattern[0]);
const size_t chunk_size = 64; // Adjust this to the size of each chunk you expect

void init_hidraw_buffer(hidraw_buffer_t* buf) {
    memset(buf->buffer, 0, sizeof(buf->buffer));
    buf->bytes_read = 0;
    pthread_mutex_init(&buf->mutex, NULL);
}
void destroy_hidraw_buffer(hidraw_buffer_t* buf) {
    pthread_mutex_destroy(&buf->mutex);
}
void print_bytes(const unsigned char* buffer, size_t size) {
    printf("Data (%zu bytes): ", size);
    for (size_t i = 0; i < size; ++i) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}
void process_data(unsigned char* data, ssize_t size) {
    // Iterate through the data buffer
    for (ssize_t i = 0; i <= size - chunk_size; ++i) {
        // Check if the pattern matches
        if (memcmp(&data[i], pattern, pattern_size) == 0) {
            // Found the pattern, process the next chunk_size bytes
            printf("Matched pattern at index %zd: ", i);
            for (size_t j = 0; j < chunk_size; ++j) {
                printf("%02x ", data[i + j]);
            }
            printf("\n");

            // Skip the processed chunk
            i += chunk_size - 1;
        }
    }
}
typedef struct {
    int fd; // File descriptor for the HIDRAW device
    // Add other members as needed for the specific device
} dev_hidraw_t;
int dev_hidraw_read(int  fd, hidraw_message_t *const out){
    if(fd < 0 || out == NULL) {
        return -1; //invalid args
    }
    ssize_t bytes = read(fd, out->data, HIDRAW_DATA_SIZE);
    if(bytes > 0){
        // if (bytes == HIDRAW_DATA_SIZE) {

        // }
        out->data_size = bytes;
        // print_bytes(bytes, HIDRAW_DATA_SIZE);
        return 0;//Sucess
    } else  if (bytes == 1 && errno != EAGAIN){
        perror("Read error");
        return -errno;
    }
    return 0; //No data read, but no error
}
void* hidraw_reading_thread(void* ptr){
    struct input_ctx* ctx = (struct input_ctx*)ptr;
    // printf("Thread started, ctx: %p\n", ctx);
    if (!ctx) {
        fprintf(stderr, "Context is NULL\n");
        return NULL;
    }

    int fd = open("/dev/hidraw2", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open /dev/hidraw2");
        return NULL;
    }

    int rc;
    int termination_condition = 1;
    // while(!ctx->termination_condition) {   
    message_t* msg = NULL;
    // message_t* msg = get_free_msg
    while(termination_condition == 1) {  
        if (msg == NULL) {
            for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
                if ((ctx->messages[h].flags & MESSAGE_FLAGS_HANDLE_DONE)) {
                    msg = &ctx->messages[h];
                    break;
                }
            }
        }
        if (msg == NULL) {
            usleep(10000); // Wait a bit before retrying
            continue;
        }

        int  rc = dev_hidraw_read(fd, &msg->data.hidraw);
        if(rc == 0){
            msg->type = MSG_TYPE_HIDRAW;
            msg->flags = 0; //Reset
            if(queue_push(ctx->queue, (void*)msg)!=0){
                fprintf(stderr, "Error pushing HIDRAW event\n");
            }
            msg=NULL;
        } else {
            msg->flags |= MESSAGE_FLAGS_HANDLE_DONE;
            msg = NULL;
        }
        usleep(15000);
    }
    
    close(fd);
    return NULL;
    //memory leak go brrrr
}
void print_hex(const unsigned char *data, ssize_t size) {
    printf("Data (%zd bytes): ", size);
    for (ssize_t i = 0; i < size; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

size_t poll_hidraw_data(hidraw_buffer_t* shared_buffer, unsigned char* out_buffer, size_t max_size) {
    pthread_mutex_lock(&shared_buffer->mutex);
    size_t bytes_to_copy = (shared_buffer->buffer, bytes_to_copy);
    memcpy(out_buffer, shared_buffer->buffer, bytes_to_copy);
    pthread_mutex_unlock(&shared_buffer->mutex);

    // Print the size of data being copied
    printf("Copying %zu bytes from shared buffer\n", bytes_to_copy);

    return bytes_to_copy;
}

static void input_hidraw(
    input_dev_t *const in_dev,
    struct input_ctx *const ctx
) {
    static const char *hidraw_path = "/dev/hidraw2";

    for (;;) {
        if (logic_termination_requested(in_dev->logic)) {
            // Break the loop if termination is requested
            break;
        }

        // Lock the mutex for exclusive access to input resources
        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            fprintf(stderr, "Cannot lock input mutex: %d, will retry later...\n", input_acquire_lock_result);
            usleep(150000);
            continue;
        }

        // Opening the HIDRAW device
        int fd = open(hidraw_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            fprintf(stderr, "Failed to open HIDRAW device at %s.\n", hidraw_path);
            pthread_mutex_unlock(&input_acquire_mutex);
            usleep(250000);  // Wait a bit before retrying
            continue;
        }
        // printf("Opened HIDRAW device at %s.\n", hidraw_path);

        // Release the mutex after successfully opening the device
        pthread_mutex_unlock(&input_acquire_mutex);

        // Create and start the HIDRAW reading thread
        pthread_t hidraw_thread;
        int ret = pthread_create(&hidraw_thread, NULL, hidraw_reading_thread, ctx);
        if (ret != 0) {
            fprintf(stderr, "Failed to create HIDRAW reading thread.\n");
            close(fd);
            continue;
        }

        // Wait for the HIDRAW reading thread to finish
        pthread_join(hidraw_thread, NULL);

        // Close the HIDRAW device
        close(fd);
    }

}

static void input_iio(
    input_dev_t *const in_dev,
    struct input_ctx *const ctx
) {
    int open_sysfs_idx = -1;

    for (;;) {
        if (logic_termination_requested(in_dev->logic)) {
            break;
        }

        // clean up from previous iteration
        if (ctx->iio_dev != NULL) {
            dev_iio_destroy(ctx->iio_dev);
            ctx->dev = NULL;
        }
        
        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            fprintf(stderr, "Cannot lock input mutex: %d, will retry later...\n", input_acquire_lock_result);
            usleep(150000);
            continue;
        }

        // clean up leftover from previous opening
        if (open_sysfs_idx >= 0) {
            free(open_sysfs[open_sysfs_idx]);
            open_sysfs[open_sysfs_idx] = NULL;
        }

        char path[512] = "\0";
        
        DIR *d;
        struct dirent *dir;
        d = opendir(iio_path);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.') {
                    continue;
                } else if (dir->d_name[0] == 'b') { // by-id
                    continue;
                } else if (dir->d_name[0] == 'j') { // js-0
                    continue;
                }

                sprintf(path, "%s%s", iio_path, dir->d_name);

                // check if that has been already opened
                // open_sysfs
                int skip = 0;
                for (int o = 0; o < (sizeof(open_sysfs) / sizeof(const char*)); ++o) {
                    if ((open_sysfs[o] != NULL) && (strcmp(open_sysfs[o], path) == 0)) {
                        fprintf(stderr, "already opened iio device %s: skip.\n", path);
                        skip = 1;
                        break;
                    }
                }

                if (skip) {
                    continue;
                }

                // try to open the device
                ctx->iio_dev = iio_matches(path, in_dev->iio_filters);
                if (ctx->iio_dev != NULL) {
                    open_sysfs_idx = 0;
                    while (open_sysfs[open_sysfs_idx] != NULL) {
                        ++open_sysfs_idx;
                    }
                    open_sysfs[open_sysfs_idx] = malloc(sizeof(path));
                    memcpy(open_sysfs[open_sysfs_idx], path, 512);    

                    printf("Opened iio %s\n    name: %s\n",
                        path,
                        dev_iio_get_name(ctx->iio_dev)
                    );
                    
                    break;
                } else {
                    fprintf(stderr, "iio device in %s does NOT matches\n", path);
                    ctx->iio_dev = NULL;
                }
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        // if device was not open "continue"
        if (ctx->iio_dev == NULL) {
            usleep(250000);
            continue;
        }

        pthread_t incoming_events_thread;

        const int incoming_events_thread_creation = pthread_create(&incoming_events_thread, NULL, iio_read_thread_func, (void*)ctx);
        if (incoming_events_thread_creation != 0) {
            fprintf(stderr, "Error creating the input thread for device %s: %d\n", dev_iio_get_name(ctx->iio_dev), incoming_events_thread_creation);
        }

        if (incoming_events_thread_creation == 0) {
            pthread_join(incoming_events_thread, NULL);
        }
    }
}

static void input_udev(
    input_dev_t *const in_dev,
    struct input_ctx *const ctx
) {
    int open_sysfs_idx = -1;

    for (;;) {
        if (logic_termination_requested(in_dev->logic)) {
            break;
        }

        // clean up from previous iteration
        if (ctx->dev != NULL) {
            libevdev_free(ctx->dev);
            ctx->dev = NULL;
        }

        const int input_acquire_lock_result = pthread_mutex_lock(&input_acquire_mutex);
        if (input_acquire_lock_result != 0) {
            fprintf(stderr, "Cannot lock input mutex: %d, will retry later...\n", input_acquire_lock_result);
            usleep(250000);
            continue;
        }

        // clean up leftover from previous opening
        if (open_sysfs_idx >= 0) {
            free(open_sysfs[open_sysfs_idx]);
            open_sysfs[open_sysfs_idx] = NULL;
        }

        char path[512] = "\0";
        
        DIR *d;
        struct dirent *dir;
        d = opendir(input_path);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_name[0] == '.') {
                    continue;
                } else if (dir->d_name[0] == 'b') { // by-id
                    continue;
                } else if (dir->d_name[0] == 'j') { // js-0
                    continue;
                }

                sprintf(path, "%s%s", input_path, dir->d_name);

                // check if that has been already opened
                // open_sysfs
                int skip = 0;
                for (int o = 0; o < (sizeof(open_sysfs) / sizeof(const char*)); ++o) {
                    if ((open_sysfs[o] != NULL) && (strcmp(open_sysfs[o], path) == 0)) {
                        skip = 1;
                        break;
                    }
                }

                if (skip) {
                    continue;
                }

                // try to open the device
                ctx->dev = ev_matches(path, in_dev->ev_filters);
                if (ctx->dev != NULL) {
                    open_sysfs_idx = 0;
                    while (open_sysfs[open_sysfs_idx] != NULL) {
                        ++open_sysfs_idx;
                    }
                    open_sysfs[open_sysfs_idx] = malloc(sizeof(path));
                    memcpy(open_sysfs[open_sysfs_idx], path, 512);    

                    if (libevdev_has_event_type(ctx->dev, EV_FF)) {
                        printf("Opened device %s\n    name: %s\n    rumble: %s\n",
                            path,
                            libevdev_get_name(ctx->dev),
                            libevdev_has_event_code(ctx->dev, EV_FF, FF_RUMBLE) ? "true" : "false"
                        );
                    } else {
                        printf("Opened device %s\n    name: %s\n    rumble: no EV_FF\n",
                            path,
                            libevdev_get_name(ctx->dev)
                        );
                    }
                    
                    break;
                }
            }
            closedir(d);
        }

        pthread_mutex_unlock(&input_acquire_mutex);

        if (ctx->dev == NULL) {
            usleep(250000);
            continue;
        }

        const int fd = libevdev_get_fd(ctx->dev);

        struct ff_effect current_effect = {
            .type = FF_RUMBLE,
            .id = -1,
            .replay = {
                .delay = 0,
                .length = 5000,
            },
            .u = {
                .rumble = {
                    .strong_magnitude = 0x0000,
                    .weak_magnitude = 0x0000,
                }
            }
        };

        // start the incoming events read thread
        pthread_t incoming_events_thread;
        const int incoming_events_thread_creation = pthread_create(&incoming_events_thread, NULL, input_read_thread_func, (void*)ctx);
        if (incoming_events_thread_creation != 0) {
            fprintf(stderr, "Error creating the input thread for device %s: %d\n", libevdev_get_name(ctx->dev), incoming_events_thread_creation);
            continue;
        }

        const int has_ff = libevdev_has_event_type(ctx->dev, EV_FF);

        if (has_ff) {
            const struct input_event gain = {
                .type = EV_FF,
                .code = FF_GAIN,
                .value = ctx->settings->ff_gain,
            };

            const int gain_set_res = write(fd, (const void*)&gain, sizeof(gain));
            if (gain_set_res != sizeof(gain)) {
                fprintf(stderr, "Unable to adjust gain for force-feedback: %d\n", gain_set_res);
            } else {
                printf("Gain for force-feedback set to %u\n", gain.value);
            }
        }

        const int timeout_ms = 1200;

        // while the incoming events thread run...
        while ((ctx->flags & INPUT_CTX_FLAGS_READ_TERMINATED) == 0) {

            if (has_ff) {
                
                void* rmsg = NULL;

                const int rumble_msg_recv_res = queue_pop_timeout(ctx->rumble_queue, &rmsg, timeout_ms);
                if (rumble_msg_recv_res == 0) {
                    rumble_message_t *const rumble_msg = (rumble_message_t*)rmsg;

                    // here stop the previous rumble
                    if (current_effect.id != -1) {
                        struct input_event rumble_stop = {
                            .type = EV_FF,
                            .code = current_effect.id,
                            .value = 0,
                        };

                        const int rumble_stop_res = write(fd, (const void*) &rumble_stop, sizeof(rumble_stop));
                        if (rumble_stop_res != sizeof(rumble_stop)) {
                            fprintf(stderr, "Unable to stop the previous rumble: %d\n", rumble_stop_res);
                        }
                    }

                    current_effect.u.rumble.strong_magnitude = rumble_msg->strong_magnitude;
                    current_effect.u.rumble.weak_magnitude = rumble_msg->weak_magnitude;

#if defined(INCLUDE_INPUT_DEBUG)
                    printf("Rumble event received -- strong_magnitude: %u, weak_magnitude: %u\n", (unsigned)current_effect.u.rumble.strong_magnitude, (unsigned)current_effect.u.rumble.weak_magnitude);
#endif

                    const int effect_upload_res = ioctl(fd, EVIOCSFF, &current_effect);
                    if (effect_upload_res == 0) {
                        const struct input_event rumble_play = {
                            .type = EV_FF,
                            .code = current_effect.id,
                            .value = 1,
                        };

                        const int effect_start_res = write(fd, (const void*)&rumble_play, sizeof(rumble_play));
                        if (effect_start_res == sizeof(rumble_play)) {
#if defined(INCLUDE_INPUT_DEBUG)
                            printf("Rumble effect play requested to driver\n");
#endif
                        } else {
                            fprintf(stderr, "Unable to write input event starting the rumble: %d\n", effect_start_res);
                        }
                    } else {
                        fprintf(stderr, "Unable to update force-feedback effect: %d\n", effect_upload_res);

                        current_effect.id = -1;
                    }

                    // this message was allocated by output_dev so I have to free it
                    free(rumble_msg);
                }
            } else {
                usleep(timeout_ms * 1000);
            }

        }



        // stop any effect
        if ((has_ff) && (current_effect.id != -1)) {
            const int effect_removal_res = ioctl(fd, EVIOCRMFF, current_effect.id);
            if (effect_removal_res == 0) {
                printf("\n");
            } else {
                fprintf(stderr, "Error removing rumble effect: %d\n", effect_removal_res);
            }
        }

        // wait for incoming events thread to totally stop 
        pthread_join(incoming_events_thread, NULL);
    }
}

void *input_dev_thread_func(void *ptr) {
    input_dev_t *in_dev = (input_dev_t*)ptr;

    struct input_ctx ctx = {
        .dev = NULL,
        .queue = &in_dev->logic->input_queue,
        .rumble_queue = &in_dev->logic->rumble_events_queue,
        .settings = &in_dev->logic->controller_settings,
        .input_filter_fn = in_dev->ev_input_filter_fn,
        .flags = 0x00000000U
    };

    if (in_dev->dev_type == input_dev_type_uinput) {
        // prepare space and empty messages
        for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
            ctx.messages[h].flags = MESSAGE_FLAGS_HANDLE_DONE;
            ctx.messages[h].type = MSG_TYPE_EV;
            ctx.messages[h].data.event.ev_size = DEFAULT_EVENTS_IN_REPORT;
            ctx.messages[h].data.event.ev = malloc(sizeof(struct input_event) * ctx.messages[h].data.event.ev_size);
        }

        input_udev(in_dev, &ctx);
    } else if (in_dev->dev_type == input_dev_type_iio) {
        // prepare space and empty messages
        for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
            ctx.messages[h].flags = MESSAGE_FLAGS_HANDLE_DONE;
            ctx.messages[h].type = MSG_TYPE_IMU;
        }

        input_iio(in_dev, &ctx);
    } else if (in_dev->dev_type == input_dev_type_hidraw) {
        for (int h = 0; h < MAX_MESSAGES_IN_FLIGHT; ++h) {
            ctx.messages[h].flags = MESSAGE_FLAGS_HANDLE_DONE;
            ctx.messages[h].type = MSG_TYPE_IMU;
        }
        input_hidraw(in_dev, &ctx);

    }
    
    return NULL;
}
