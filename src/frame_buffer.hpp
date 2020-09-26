#ifndef FRAME_BUFFER
#define FRAME_BUFFER

#include <memory>
#include <array>
#include <mutex>
#include <functional>
#include <condition_variable>

#define BUFFER_SIZE 3

enum struct FrameFlag {
    READABLE,
    WRITABLE
};

struct FrameBuffer {
    std::array<std::unique_ptr<uint8_t[]>, BUFFER_SIZE> buffer;
    std::array<FrameFlag, BUFFER_SIZE> flags;
    uint8_t write_index;
    uint8_t read_index;
    std::mutex mut;
    std::condition_variable reader_condvar;
    std::condition_variable writer_condvar;

    FrameBuffer (int frame_width, int frame_height) : write_index(0), read_index(0) {
        for (auto &frame_data_ptr: buffer) {
            frame_data_ptr = std::make_unique<uint8_t[]>(frame_width * frame_height * 4);
        }
        for (auto &flag: flags) {
            flag = FrameFlag::WRITABLE;
        }
    }

    auto write_frame (std::function<void (uint8_t*)> update_frame_cb) {
        std::unique_lock<std::mutex> lock(mut);

        writer_condvar.wait(lock, [this](){return flags[write_index] == FrameFlag::WRITABLE;});

        update_frame_cb(buffer[write_index].get());

        write_index = ++write_index < BUFFER_SIZE ? write_index : 0;
        flags[write_index] = FrameFlag::READABLE;

        reader_condvar.notify_one();
    }

    auto read_frame (std::function<void (uint8_t*)> read_frame_cb) {
        std::unique_lock<std::mutex> lock(mut);

        reader_condvar.wait(lock, [this](){return flags[write_index] == FrameFlag::READABLE;});

        read_frame_cb(buffer[read_index].get());

        read_index = ++read_index < BUFFER_SIZE ? read_index : 0;
        flags[write_index] = FrameFlag::WRITABLE;

        writer_condvar.notify_one();
    }
};

#endif