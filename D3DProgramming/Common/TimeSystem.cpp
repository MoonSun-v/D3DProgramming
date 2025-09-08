#include "TimeSystem.h"
#include <windows.h>


// 정적 싱글톤 인스턴스 초기화
TimeSystem* TimeSystem::m_Instance = nullptr;


TimeSystem::TimeSystem() : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{

	// 타이머 주파수(1초당 카운트 횟수) 가져오기 
	__int64 countsPerSec;					
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);


	// 1카운트가 몇 초인지
	mSecondsPerCount = 1.0 / (double)countsPerSec;	


	// 싱글톤 인스턴스 등록
	if (m_Instance == nullptr)
	{
		m_Instance = this;
	}

}



// 총 경과 시간 반환 (일시정지 시간 제외)
float TimeSystem::TotalTime()const
{
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime


	// 스톱 상태일 때 : (mStopTime - mPausedTime) - mBaseTime 만큼의 시간을 계산
	if (mStopped)
	{
		return (float)(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}

	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime


	// 실행 중일 때: (mCurrTime - mPausedTime) - mBaseTime
	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}


// 델타 타임 반환 (프레임 간격)
float TimeSystem::DeltaTime()const
{
	return (float)mDeltaTime;
}


// 타이머 리셋 
void TimeSystem::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}


// 타이머 시작 
void TimeSystem::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if (mStopped)
	{
		mPausedTime += (startTime - mStopTime);	 // 정지된 시간만큼을 mPausedTime에 누적

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}


// 타이머 정지 (일시정지 시 호출)
void TimeSystem::Stop()
{
	if (!mStopped)
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;
		mStopped = true;
	}
}


// 매 프레임마다 시간 갱신
void TimeSystem::Tick()
{
	// 정지 상태에서는 델타 타임을 0으로 설정
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	// 현재 시간 가져오기
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// 이번 프레임 - 이전 프레임의 시간 차이 계산
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// 다음 프레임을 위해 이전 시간 갱신
	mPrevTime = mCurrTime;


	// 델타 타임이 음수가 되는 예외 상황 보정 (전원 관리 모드 등)
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}




