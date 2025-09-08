#include "TimeSystem.h"
#include <windows.h>


// ���� �̱��� �ν��Ͻ� �ʱ�ȭ
TimeSystem* TimeSystem::m_Instance = nullptr;


TimeSystem::TimeSystem() : mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{

	// Ÿ�̸� ���ļ�(1�ʴ� ī��Ʈ Ƚ��) �������� 
	__int64 countsPerSec;					
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);


	// 1ī��Ʈ�� �� ������
	mSecondsPerCount = 1.0 / (double)countsPerSec;	


	// �̱��� �ν��Ͻ� ���
	if (m_Instance == nullptr)
	{
		m_Instance = this;
	}

}



// �� ��� �ð� ��ȯ (�Ͻ����� �ð� ����)
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


	// ���� ������ �� : (mStopTime - mPausedTime) - mBaseTime ��ŭ�� �ð��� ���
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


	// ���� ���� ��: (mCurrTime - mPausedTime) - mBaseTime
	else
	{
		return (float)(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	}
}


// ��Ÿ Ÿ�� ��ȯ (������ ����)
float TimeSystem::DeltaTime()const
{
	return (float)mDeltaTime;
}


// Ÿ�̸� ���� 
void TimeSystem::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = false;
}


// Ÿ�̸� ���� 
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
		mPausedTime += (startTime - mStopTime);	 // ������ �ð���ŭ�� mPausedTime�� ����

		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
}


// Ÿ�̸� ���� (�Ͻ����� �� ȣ��)
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


// �� �����Ӹ��� �ð� ����
void TimeSystem::Tick()
{
	// ���� ���¿����� ��Ÿ Ÿ���� 0���� ����
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	// ���� �ð� ��������
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// �̹� ������ - ���� �������� �ð� ���� ���
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// ���� �������� ���� ���� �ð� ����
	mPrevTime = mCurrTime;


	// ��Ÿ Ÿ���� ������ �Ǵ� ���� ��Ȳ ���� (���� ���� ��� ��)
	if (mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}




