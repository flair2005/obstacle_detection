#include "OpenGL.h"
#include "GLWindow2.h"
#include "GLWindowMenu.h"
#include <stdlib.h>
#include <gvars3/GStringUtil.h>
#include <gvars3/instances.h>
#include <TooN/helpers.h>

using namespace CVD;
using namespace std;
using namespace GVars3;
using namespace TooN;

GLWindow2::GLWindow2(ImageRef irSize, string sTitle)
  : GLWindow(irSize, sTitle)
{

#ifdef WIN32
  // On windows, have to initialise GLEW at the start to enable access
  // to GL extensions
  static bool bGLEWIsInit = false;
  if(!bGLEWIsInit)
  {
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
		exit(0);
	}
	bGLEWIsInit = true;
  }
#endif

  mirVideoSize = irSize;
  GUI.RegisterCommand("GLWindow.AddMenu", GUICommandCallBack, this);
  glSetFont("sans");
  mvMCPoseUpdate=Zeros;
  mvLeftPoseUpdate=Zeros;
};


void GLWindow2::AddMenu(string sName, string sTitle)
{
  GLWindowMenu* pMenu = new GLWindowMenu(sName, sTitle); 
  mvpGLWindowMenus.push_back(pMenu);
}

void GLWindow2::GUICommandCallBack(void* ptr, string sCommand, string sParams)
{
  ((GLWindow2*) ptr)->GUICommandHandler(sCommand, sParams);
}

void GLWindow2::GUICommandHandler(string sCommand, string sParams)  // Called by the callback func..
{
  vector<string> vs=ChopAndUnquoteString(sParams);
  if(sCommand=="GLWindow.AddMenu")
    {
      switch(vs.size())
	{
	case 1:
	  AddMenu(vs[0], "Root");
	  return;
	case 2:
	  AddMenu(vs[0], vs[1]);
	  return;
	default:
	  cout << "? AddMenu: need one or two params (internal menu name, [caption])." << endl;
	  return;
	};
    };
  
  // Should have returned to caller by now - if got here, a command which 
  // was not handled was registered....
  cout << "! GLWindow::GUICommandHandler: unhandled command "<< sCommand << endl;
  exit(1);
}; 

void GLWindow2::DrawMenus()
{
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_POLYGON_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColorMask(1,1,1,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  SetupWindowOrtho();
  glLineWidth(1);
  
  int nTop = 30;
  int nHeight = 30;
  for(vector<GLWindowMenu*>::iterator i = mvpGLWindowMenus.begin();
      i!= mvpGLWindowMenus.end();
      i++)
    {
      (*i)->Render(nTop, nHeight, size()[0], *this);
      nTop+=nHeight+1;
    }
  
}

void GLWindow2::SetupUnitOrtho()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,1,1,0,0,1);
}

void GLWindow2::SetupWindowOrtho()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(size());
}

void GLWindow2::SetupVideoOrtho()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-0.5,(double)mirVideoSize.x - 0.5, (double) mirVideoSize.y - 0.5, -0.5, -1.0, 1.0);
}

void GLWindow2::SetupVideoRasterPosAndZoom()
{ 
  glRasterPos2d(-0.5,-0.5);
  double adZoom[2];
  adZoom[0] = (double) size()[0] / (double) mirVideoSize[0];
  adZoom[1] = (double) size()[1] / (double) mirVideoSize[1];
  glPixelZoom(adZoom[0], -adZoom[1]);
}

void GLWindow2::SetupViewport()
{
  glViewport(0, 0, size()[0], size()[1]);
}

void GLWindow2::PrintString(CVD::ImageRef irPos, std::string s)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glTranslatef(irPos.x, irPos.y, 0.0);
  glScalef(8,-8,1);
  glDrawText(s, NICE, 1.6, 0.1);
  glPopMatrix();
}

void GLWindow2::DrawCaption(string s)
{
  if(s.length() == 0)
    return;
  
  SetupWindowOrtho();
  // Find out how many lines are in the caption:
  // Count the endls
  int nLines = 0;
  {
    string sendl("\n");
    string::size_type st=0;
    while(1)
      {
	nLines++;
	st = s.find(sendl, st);
	if(st==string::npos)
	  break;
	else
	  st++;
      }
  }
  
  int nTopOfBox = size().y - nLines * 17;
  
  // Draw a grey background box for the text
  glColor4f(0,0,0,0.4);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  glVertex2d(-0.5, nTopOfBox);
  glVertex2d(size().x, nTopOfBox);
  glVertex2d(size().x, size().y);
  glVertex2d(-0.5, size().y);
  glEnd();
  
  // Draw the caption text in yellow
  glColor3f(1,1,0);      
  PrintString(ImageRef(10,nTopOfBox + 13), s);
}


void GLWindow2::HandlePendingEvents()
{
  handle_events(*this);
}

void GLWindow2::on_mouse_move(GLWindow& win, CVD::ImageRef where, int state)
{
  ImageRef irMotion = where - mirLastMousePos;
  mirLastMousePos = where;
  
  double dSensitivity = 0.01;
  if(state & BUTTON_LEFT && ! (state & BUTTON_RIGHT))
    {
      mvMCPoseUpdate[3] -= irMotion[1] * dSensitivity;
      mvMCPoseUpdate[4] += irMotion[0] * dSensitivity;
    }
  else if(!(state & BUTTON_LEFT) && state & BUTTON_RIGHT)
    {
      mvLeftPoseUpdate[4] -= irMotion[0] * dSensitivity;
      mvLeftPoseUpdate[3] += irMotion[1] * dSensitivity;
    }
  else if(state & BUTTON_MIDDLE  || (state & BUTTON_LEFT && state & BUTTON_RIGHT))
    {
      mvLeftPoseUpdate[5] -= irMotion[0] * dSensitivity;
      mvLeftPoseUpdate[2] += irMotion[1] * dSensitivity;
    }
  
}

void GLWindow2::on_mouse_down(GLWindow& win, CVD::ImageRef where, int state, int button)
{
  bool bHandled = false;
  for(unsigned int i=0; !bHandled && i<mvpGLWindowMenus.size(); i++)
    bHandled = mvpGLWindowMenus[i]->HandleClick(button, state, where.x, where.y);

}

void GLWindow2::on_event(GLWindow& win, int event)
{
  if(event==EVENT_CLOSE)
    GUI.ParseLine("quit");
}

