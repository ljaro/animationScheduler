#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include <deque>
#include <queue>
#include <functional>
#include <iostream>
using namespace testing;

class Animation {
public:
    virtual ~Animation() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool running() = 0;

    virtual void setOnFinished(std::function<void()> callback) {
        onFinished = callback;
    }

    virtual void finished() {
        if(onFinished) onFinished();
    }
private:
    std::function<void()> onFinished;
};

class MockAnimation : public Animation {
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(running, bool());
};

using NiceAnimation = NiceMock<MockAnimation>;

class Scheduler {
public:
    void scheduleAnimation(Animation* anim, std::function<void()> callback = {})
    {
        if(currentIdx == 0) {
            queue.push_back(std::make_pair(anim, callback));
        } else {
            queue.insert(queue.begin() + currentIdx, std::make_pair(anim, callback));
            currentIdx++;
        }
        schedule();
    }

    void schedule() {
        if(!queue.empty()) {
            auto& current = queue.front();
            if(!current.first->running()) {
                current.first->start();
                current.first->setOnFinished([&](){
                    auto& finishedAnimObj = queue.front();
                    if(finishedAnimObj.second) {
                        currentIdx++;
                        finishedAnimObj.second();
                        currentIdx = 0;
                    }
                    queue.pop_front();
                    schedule();
                });
            }
        }
    }
private:
    using AnimObject = std::pair<Animation*, std::function<void()>>;
    std::deque<AnimObject> queue;
    int currentIdx = 0;
};

TEST(AnimScheduler, should_start_animation_after_scheduleAnimation_call)
{
    NiceAnimation anim1;

    Scheduler s;

    ON_CALL(anim1, running()).WillByDefault(Return(false));
    EXPECT_CALL(anim1, start());

    s.scheduleAnimation(&anim1);

}

TEST(AnimScheduler, should_queue_animation_but_not_run_if_already_running)
{
    NiceAnimation anim1;
    NiceAnimation anim2;

    Scheduler s;

    ON_CALL(anim1, running()).WillByDefault([](){
        static int cnt = 0;
        bool arr[] = {false, true};
        return arr[cnt++];
    });

    EXPECT_CALL(anim1, start());
    EXPECT_CALL(anim2, start()).Times(0);

    s.scheduleAnimation(&anim1);
    s.scheduleAnimation(&anim2);
}

TEST(AnimScheduler, should_start_another_if_prev_finished)
{
    NiceAnimation anim1;
    NiceAnimation anim2;

    Scheduler s;

    EXPECT_CALL(anim1, start()).Times(1);
    EXPECT_CALL(anim2, start()).Times(1);

    ON_CALL(anim1, running()).WillByDefault(Return(false));
    s.scheduleAnimation(&anim1);

    ON_CALL(anim1, running()).WillByDefault(Return(true));
    s.scheduleAnimation(&anim2);


    ON_CALL(anim2, running()).WillByDefault(Return(false));

    anim1.finished();
}

TEST(AnimScheduler, should_call_callback)
{
    NiceAnimation anim1;

    Scheduler s;

    ON_CALL(anim1, running()).WillByDefault(Return(false));

    bool invoked = false;
    s.scheduleAnimation(&anim1, [&](){
        invoked = true;
    });

    anim1.finished();

    ASSERT_TRUE(invoked);

}

TEST(AnimScheduler, should_respect_order_of_animation_from_callback)
{
    NiceAnimation anim1;
    NiceAnimation animFromCallback;
    NiceAnimation anim2;

    Scheduler s;

    {
        InSequence seq;
        EXPECT_CALL(anim1, start());
        EXPECT_CALL(animFromCallback, start());
        EXPECT_CALL(anim2, start());
    }

    ON_CALL(anim1, running()).WillByDefault(Return(false));
    s.scheduleAnimation(&anim1, [&](){
        ON_CALL(anim1, running()).WillByDefault(Return(true));
        s.scheduleAnimation(&animFromCallback);
    });

    ON_CALL(anim1, running()).WillByDefault(Return(true));
    s.scheduleAnimation(&anim2);


    anim1.finished();
    animFromCallback.finished();
    anim2.finished();
}

TEST(AnimScheduler, should_respect_order_of_animation_from_callback_complex1)
{
    NiceAnimation anim1;
    NiceAnimation animFromCallback11;
    NiceAnimation animFromCallback12;
    NiceAnimation anim2;
    NiceAnimation animFromCallback2;

    {
        InSequence seq;
        EXPECT_CALL(anim1, start());
        EXPECT_CALL(animFromCallback11, start());
        EXPECT_CALL(animFromCallback12, start());
        EXPECT_CALL(anim2, start());
        EXPECT_CALL(animFromCallback2, start());
    }

    Scheduler s;



    ON_CALL(anim1, running()).WillByDefault(Return(false));

    s.scheduleAnimation(&anim1, [&](){
        ON_CALL(anim1, running()).WillByDefault(Return(true));
        s.scheduleAnimation(&animFromCallback11);
        s.scheduleAnimation(&animFromCallback12);
    });

    ON_CALL(anim1, running()).WillByDefault(Return(true));

    s.scheduleAnimation(&anim2, [&](){
        ON_CALL(anim2, running()).WillByDefault(Return(true));
        s.scheduleAnimation(&animFromCallback2);
    });

    anim1.finished();
    animFromCallback11.finished();
    animFromCallback12.finished();
    anim2.finished();
    animFromCallback2.finished();
}

