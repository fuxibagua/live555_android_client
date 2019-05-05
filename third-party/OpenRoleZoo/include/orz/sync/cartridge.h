#ifndef ORZ_SYNC_BULLET_H
#define ORZ_SYNC_BULLET_H

#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace orz {

    class Cartridge {
    public:
        Cartridge();

        ~Cartridge();

        using bullet_type = std::function<void(int)>;
        using shell_type = std::function<void(int)>;

        /**
         * @brief fire 异步组装发射子弹，先调用弹头，然后调用弹壳
         * @param signet 表示子弹索引号
         * @param bullet 先调用函数
         * @param shell 后调用函数
         */
        void fire(int signet, const bullet_type &bullet, const shell_type &shell = nullptr);

        bool busy();

        void join();

    private:
        Cartridge(const Cartridge &that) = delete;
        const Cartridge &operator=(const Cartridge &that) = delete;

        void operating();

        std::mutex fire_mutex;              /// 总体锁，对子弹整体加锁
        std::condition_variable fire_cond;  /// 子弹状态切换
        std::atomic<bool> dry;              /// 子弹是干燥的才能发射，子弹是否使能

        int signet;                         /// 子弹标记
        bullet_type bullet = nullptr;       /// 子弹头
        shell_type shell = nullptr;         /// 子弹壳

        std::thread powder;                 /// 工作线程

    };

}

#endif // ORZ_SYNC_BULLET_H
