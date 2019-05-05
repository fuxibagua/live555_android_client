#ifndef ORZ_SYNC_SHOTGUN_H
#define ORZ_SYNC_SHOTGUN_H

#include "cartridge.h"

#include <vector>
#include <deque>

namespace orz {

/**
 * @brief The Shotgun class 多线程异步操作封装类，可以将提交的任务分配给空闲的线程调用
 */
    class Shotgun {
    public:
        /**
         * @brief Shotgun
         * @param clip_size 该霰弹枪中子弹的个数
         */
        Shotgun(size_t clip_size);

        ~Shotgun();

        /**
         * @brief fire 根据子弹头组合出空闲的子弹，然后发射
         * @param bullet 准备发射的子弹头
         * @return 发射出去的子弹
         */
        Cartridge *fire(const Cartridge::bullet_type &bullet);

        /**
         * @brief fire 根据子弹头组合出空闲的子弹，然后发射
         * @param bullet 准备发射的子弹头
         * @param shell 额外的弹壳
         * @return 发射出去的子弹
         */
        Cartridge *fire(const Cartridge::bullet_type &bullet, const Cartridge::shell_type &shell);

        /**
         * @brief join 等待所有子弹回收完毕后返回
         */
        void join();

        /**
         * @brief busy 返回当前是否有任务进行
         * @return 有任务进行返回真
         */
        bool busy();

        /**
         * @brief size 获取弹夹大小
         * @return 弹夹大小
         */
        size_t size() const;

    private:
        Shotgun(const Shotgun &that) = delete;
        const Shotgun &operator=(const Shotgun &that) = delete;

        /**
         * @brief load 上子弹，准备发射
         * @return 可以发射的子弹对应的标记
         */
        int load();

        /**
         * @brief recycling_cartridge 回收子弹放入枪膛
         * @param signet 待回收的子弹
         */
        void recycling_cartridge(int signet);

        std::vector<Cartridge *> clip;      /// 存放所有子弹资源

        std::mutex chest_mutex;                 /// 枪膛的锁
        std::condition_variable chest_cond;     /// 子弹上膛的信号
        std::deque<int> chest;                  /// 存放所有上膛的子弹的标记
    };

}

#endif // ORZ_SYNC_SHOTGUN_H
