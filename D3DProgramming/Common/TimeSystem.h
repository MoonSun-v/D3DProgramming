#pragma once

class TimeSystem
{
public:
	TimeSystem();

	float TotalTime()const;			// ���α׷� ���� ���� ����� �� �ð�(�� ����)
	float DeltaTime()const;			// ���� �����Ӱ� ���� ������ ������ �ð� ����(�� ����)

	void Reset();					// Ÿ�̸� ���� (���� ���� ���� �� �ݵ�� ȣ��)
	void Start();					// Ÿ�̸� ���� (�Ͻ����� ���¿��� �ٽ� ������ �� ȣ��)
	void Stop();					// Ÿ�̸� ���� (�Ͻ����� �� ȣ��)
	void Tick();					// �� �����Ӹ��� ȣ��

	static TimeSystem* m_Instance;	// �̱��� �ν��Ͻ� ������

private:
	double mSecondsPerCount;		// ī���� ���ļ�(1�ʴ� ī��Ʈ ��)�� ���� "1ī��Ʈ�� �ɸ��� ��" ��
	double mDeltaTime;				// DeltaTime ��� ���� (���� ������ - ���� ������ �ð���)

	__int64 mBaseTime;				// ������ �Ǵ� �ð� (Reset ����)
	__int64 mPausedTime;			// �Ͻ������� �ð� ������
	__int64 mStopTime;				// Ÿ�̸Ӱ� ������ ������ �ð�
	__int64 mPrevTime;				// ���� ������ �ð�
	__int64 mCurrTime;					

	bool mStopped;					// Ÿ�̸Ӱ� ���� �������� ���� 
};
