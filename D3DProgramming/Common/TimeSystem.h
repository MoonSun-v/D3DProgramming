#pragma once

class TimeSystem
{
public:
	TimeSystem();

	float TotalTime()const;			// 프로그램 시작 이후 경과한 총 시간(초 단위)
	float DeltaTime()const;			// 이전 프레임과 현재 프레임 사이의 시간 간격(초 단위)

	void Reset();					// 타이머 리셋 (게임 루프 시작 전 반드시 호출)
	void Start();					// 타이머 시작 (일시정지 상태에서 다시 시작할 때 호출)
	void Stop();					// 타이머 정지 (일시정지 시 호출)
	void Tick();					// 매 프레임마다 호출

	static TimeSystem* m_Instance;	// 싱글톤 인스턴스 포인터

private:
	double mSecondsPerCount;		// 카운터 주파수(1초당 카운트 수)에 따른 "1카운트당 걸리는 초" 값
	double mDeltaTime;				// DeltaTime 결과 저장 (현재 프레임 - 이전 프레임 시간차)

	__int64 mBaseTime;				// 기준이 되는 시간 (Reset 시점)
	__int64 mPausedTime;			// 일시정지된 시간 누적값
	__int64 mStopTime;				// 타이머가 정지된 시점의 시간
	__int64 mPrevTime;				// 이전 프레임 시간
	__int64 mCurrTime;					

	bool mStopped;					// 타이머가 정지 상태인지 여부 
};
