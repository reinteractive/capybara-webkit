#include "PageLoadingCommand.h"
#include "Command.h"
#include "WebPage.h"

#include <iostream>

PageLoadingCommand::PageLoadingCommand(Command *command, WebPage *page, QObject *parent) : QObject(parent) {
  m_page = page;
  m_command = command;
  m_pageLoadingFromCommand = false;
  m_pageSuccess = false;
  m_pendingResponse = NULL;
  connect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadingFromCommand()));
  connect(m_page, SIGNAL(pageFinished(bool)), this, SLOT(pendingLoadFinished(bool)));
}

void PageLoadingCommand::start() {
  int timeout = m_page->getTimeout();
  if (timeout > 0) {
    QTimer::singleShot(timeout * 1000, this, SLOT(timedout()));
  }
  connect(m_command, SIGNAL(finished(Response *)), this, SLOT(commandFinished(Response *)));
  m_command->start();
};

void PageLoadingCommand::timedout() {
  if (m_pageLoadingFromCommand) {
    m_pageLoadingFromCommand = false;
    m_pageSuccess = false;

    disconnect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadingFromCommand()));
    disconnect(m_page, SIGNAL(pageFinished(bool)), this, SLOT(pendingLoadFinished(bool)));

    if (!m_pendingResponse) {
      disconnect(m_command, SIGNAL(finished(Response *)), this, SLOT(commandFinished(Response *)));
      m_command->deleteLater();
    }

    emit commandTimedOut();
   // m_page->blockSignals(true);
    m_page->triggerAction(QWebPage::Stop);
   // m_page->blockSignals(false);
  }
}

void PageLoadingCommand::pendingLoadFinished(bool success) {
  m_pageSuccess = success;
  if (m_pageLoadingFromCommand) {
    m_pageLoadingFromCommand = false;
    if (m_pendingResponse) {
      if (m_pageSuccess) {
        emit finished(m_pendingResponse);
      } else {
        QString message = m_page->failureString();
        emit finished(new Response(false, message));
      }
    }
  }
}

void PageLoadingCommand::pageLoadingFromCommand() {
  m_pageLoadingFromCommand = true;
}

void PageLoadingCommand::commandFinished(Response *response) {
  disconnect(m_page, SIGNAL(loadStarted()), this, SLOT(pageLoadingFromCommand()));
  m_command->deleteLater();
  if (m_pageLoadingFromCommand)
    m_pendingResponse = response;
  else
    emit finished(response);
}
