/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2008 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

/* $Core: libIRCDlogger */

#include "inspircd.h"

/*
 * Suggested implementation...
 *	class LogManager
 *		bool AddLogType(const std::string &type, enum loglevel, LogStream *)
 *		bool DelLogType(const std::string &type, LogStream *)
 *		Log(const std::string &type, enum loglevel, const std::string &msg)
 *		std::map<std::string, std::vector<LogStream *> > logstreams (holds a 'chain' of logstreams for each type that are all notified when a log happens)
 *
 *  class LogStream
 *		std::string type
 *      virtual void OnLog(enum loglevel, const std::string &msg)
 *
 * How it works:
 *  Modules create their own logstream types (core will create one for 'file logging' for example) and create instances of these logstream types
 *  and register interest in a certain logtype. Globbing is not here, with the exception of * - for all events.. loglevel is used to drop 
 *  events that are of no interest to a logstream.
 *
 *  When Log is called, the vector of logstreams for that type is iterated (along with the special vector for "*"), and all registered logstreams
 *  are called back ("OnLog" or whatever) to do whatever they like with the message. In the case of the core, this will write to a file.
 *  In the case of the module I plan to write (m_logtochannel or something), it will log to the channel(s) for that logstream, etc.
 *
 * NOTE: Somehow we have to let LogManager manage the non-blocking file streams and provide an interface to share them with various LogStreams,
 *       as, for example, a user may want to let 'KILL' and 'XLINE' snotices go to /home/ircd/inspircd/logs/operactions.log, or whatever. How
 *       can we accomplish this easily? I guess with a map of pre-loved logpaths, and a pointer of FILE *..
 * 
 */

bool LogManager::AddLogType(const std::string &type, LogStream *l)
{
	std::map<std::string, std::vector<LogStream *> >::iterator i = LogStreams.find(type);

	if (i != LogStreams.end())
		i->second.push_back(l);
	else
	{
		std::vector<LogStream *> v;
		v.push_back(l);
		LogStreams[type] = v;
	}

	if (type == "*")
		GlobalLogStreams.push_back(l);

	return true;
}

bool LogManager::DelLogType(const std::string &type, LogStream *l)
{
	std::map<std::string, std::vector<LogStream *> >::iterator i = LogStreams.find(type);
	std::vector<LogStream *>::iterator gi = GlobalLogStreams.begin();

	while (gi != GlobalLogStreams.end())
	{
		if ((*gi) == l)
		{
			GlobalLogStreams.erase(gi);
			break;
		}
	}

	if (i != LogStreams.end())
	{
		std::vector<LogStream *>::iterator it = i->second.begin();

		while (it != i->second.end())
		{
			if (*it == l)
			{
				i->second.erase(it);

				if (i->second.size() == 0)
				{
					LogStreams.erase(i);
				}

				delete l;
				return true;
			}

			it++;
		}
	}

	return false;
}

void LogManager::Log(const std::string &type, int loglevel, const char *fmt, ...)
{
	va_list a;
	char buf[65536];

	va_start(a, fmt);
	vsnprintf(buf, 65536, fmt, a);
	va_end(a);

	this->Log(type, loglevel, std::string(buf));
}

void LogManager::Log(const std::string &type, int loglevel, const std::string &msg)
{
	std::map<std::string, std::vector<LogStream *> >::iterator i = LogStreams.find(type);

	if (i != LogStreams.end())
	{
		std::vector<LogStream *>::iterator it = i->second.begin();

		while (it != i->second.end())
		{
			(*it)->OnLog(loglevel, msg);
			it++;
		}

		return;
	}

	std::vector<LogStream *>::iterator gi = GlobalLogStreams.begin();

	while (gi != GlobalLogStreams.end())
	{
		(*gi)->OnLog(loglevel, msg);
		gi++;
	}

	// blurp, no handler for this type
	return;
}


