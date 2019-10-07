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
    virtual int id() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool running() = 0;

    std::function<void()> onFinished;
};

class MockAnimation : public Animation {
public:
    MOCK_METHOD0(id, int());
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(running, bool());
};

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
                current.first->onFinished = [&](){
                    auto& finishedAnimObj = queue.front();
                    if(finishedAnimObj.second) {
                        currentIdx++;
                        finishedAnimObj.second();
                        currentIdx = 0;
                    }
                    queue.pop_front();
                    schedule();
                };
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
    MockAnimation anim1;

    Scheduler s;

    ON_CALL(anim1, running()).WillByDefault(Return(false));
    EXPECT_CALL(anim1, start());

    s.scheduleAnimation(&anim1);

}

TEST(AnimScheduler, should_queue_animation_but_not_run_if_already_running)
{
    MockAnimation anim1;
    MockAnimation anim2;

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
    MockAnimation anim1;
    MockAnimation anim2;

    Scheduler s;

    EXPECT_CALL(anim1, start()).Times(1);
    EXPECT_CALL(anim2, start()).Times(1);

    ON_CALL(anim1, running()).WillByDefault(Return(false));
    s.scheduleAnimation(&anim1);

    ON_CALL(anim1, running()).WillByDefault(Return(true));
    s.scheduleAnimation(&anim2);


    ON_CALL(anim2, running()).WillByDefault(Return(false));

    anim1.onFinished();
}

TEST(AnimScheduler, should_call_callback)
{
    MockAnimation anim1;

    Scheduler s;

    ON_CALL(anim1, running()).WillByDefault(Return(false));

    bool invoked = false;
    s.scheduleAnimation(&anim1, [&](){
        invoked = true;
    });

    anim1.onFinished();

    ASSERT_TRUE(invoked);

}

TEST(AnimScheduler, should_respect_order_of_animation_from_callback)
{
    MockAnimation anim1;
    MockAnimation animFromCallback;
    MockAnimation anim2;

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


    anim1.onFinished();
    animFromCallback.onFinished();
    anim2.onFinished();
}

TEST(AnimScheduler, should_respect_order_of_animation_from_callback_complex1)
{
    MockAnimation anim1;
    MockAnimation animFromCallback11;
    MockAnimation animFromCallback12;
    MockAnimation anim2;
    MockAnimation animFromCallback2;

    {
        InSequence seq;
        EXPECT_CALL(anim1, start());
        EXPECT_CALL(animFromCallback11, start());
        //EXPECT_CALL(animFromCallback12, start());
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

    anim1.onFinished();
    animFromCallback11.onFinished();
    animFromCallback12.onFinished();
    anim2.onFinished();
    animFromCallback2.onFinished();
}

