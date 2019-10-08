# animationScheduler



class Animation {};
class Scheduler {};

0. Animation class is an abstract class which contains pure virtual methods: virtual void start(), virtual void stop(), virtual bool running()
1. Scheduler class must contains public method scheduleAnimation
2. scheduler should start animation after scheduleAnimation call
3. scheduler should not start animation on scheduleAnimation call if any other is running
4. scheduler should start another animation if current finished
5. extend scheduler interface such that user will have possibility to set callback when animation finished
6. scheduler should be able to schedule animation with callback and schedule another animation in that callback
7. scheduler should respect order of animations in the way that animation scheduled in callback should start right after animation from with is called from



