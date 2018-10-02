/*
 *  Created on: 2018/09/29
 *      Author: take
 */

#pragma once


namespace MinDxr {

template<typename T>
class HandleHolder{
private:
	T* m_handle{nullptr};
public:
	HandleHolder() = default;
	HandleHolder(T* handle):m_handle(handle){
	}
	~HandleHolder(){
		if(m_handle){
			m_handle->Release();
			m_handle=nullptr;
		}
	}
	T*& Get() {
		return m_handle;
	}
	T* operator->() {
		return m_handle;
	}
};

}  // namespace MinDxr

