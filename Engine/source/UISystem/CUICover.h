#ifndef CUICover_h
#define CUICover_h

#include "CKLBUITask.h"
#include "CKLBLabelNode.h"
#include <unordered_map>

/*!
* \class CUICover_h
* \brief Cover Class
*
* CUICover_h task for UI_Cover and GL_BGBorder.
* Make a square hole in the plate polygon.
* Use cases is to make translucent masks.
*/
class CUICover : public CKLBUITask
{
	struct Rect {
		u32 x;
		u32 y;
		u32 w;
		u32 h;
	};

	friend class CKLBTaskFactory<CUICover>;
private:
	CUICover();
	virtual ~CUICover();

	bool init(CKLBUITask* parent, CKLBNode* pNode, u32 order, u32 alpha, u32 color);
	bool initCore(u32 order, u32 alpha, u32 color);
public:
	static CUICover* create(CKLBUITask* parent, CKLBNode* pNode, u32 order, u32 alpha, u32 color);
	virtual u32 getClassID();

	bool initUI(CLuaState& lua);
	void execute(u32 deltaT);
	void dieUI();

	int commandUI(CLuaState& lua, int argc, int cmd);

	inline virtual u32 getOrder() { return m_order; }

	inline virtual void setOrder(u32 order) {
		if (order != m_order) {
			m_order = order;
			REFRESH_A;
		}
	}

	inline void setAlpha(u32 alpha) {
		if (alpha != m_alpha) {
			m_alpha = alpha;
			REFRESH_A;
		}
	}
	inline u32 getAlpha() { return m_alpha; }

	inline void setU24Color(u32 color) {
		if (color != m_color) {
			m_color = color;
			REFRESH_A;
		}
	}
	inline u32 getU24Color() { return m_color; }

	inline void setColor(u32 color) {
		u32 alpha = color >> 24;
		u32 col = color & 0xFFFFFF;
		if (alpha != m_alpha) {
			m_alpha = alpha;
			REFRESH_A;
		}

		if (col != m_color) {
			m_color = col;
			REFRESH_A;
		}
	}

	inline u32 getColor() { return m_color | (m_alpha << 24); }

	// For GL_BGBorder
	inline void setTexture(const char* texture) {
		setStrC(m_texture, texture);
	}

private:
	const u32 MAX_RECTS = 10;

	const char*		m_texture;
	u32				m_color;
	u32				m_order;
	u8				m_alpha;
	u32				m_counter;

	std::unordered_map<u32, Rect>	m_rects;
	static	PROP_V2					ms_propItems[];
};


#endif // CUICover_h

