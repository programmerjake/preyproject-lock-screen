/*
 * You may use this file (get_option.h) under the terms of either the zlib license, GPLv3 license, or (at your option) any later GPL license version.
 */

/* Copyright (c) 2015 Jacob Lifshay, Fork Ltd.
 *
 * This file is part of Prey.
 *
 * Prey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Prey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Prey.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Copyright (c) 2015 Jacob Lifshay
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef GET_OPTION_H_INCLUDED
#define GET_OPTION_H_INCLUDED

#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <algorithm>

/** getopt implementation
 * @note Doesn't support getopt_long_only.
 * Doesn't support assigning to optind after calling getopt*.
 * Doesn't support the POSIXLY_CORRECT environment variable.
 * Doesn't support the "W;" option.
 * Supports other GNU extensions.
 */

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_get_option final
{
public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef std::basic_string<char_type, traits_type> string_type;
private:
    std::size_t optindInternal;
    bool startedParse;
    std::size_t firstReorderedArgument;
public:
    std::function<void(string_type)> opterr; /// error message display function; can be empty (eg. nullptr)
    int_type optopt;
    std::size_t optind;
    string_type optarg;
    bool has_arg;
    static int_type end_flag()
    {
        return traits_type::eof();
    }
    explicit basic_get_option(std::function<void(string_type)> opterr = nullptr, std::size_t optind = 1)
        : optindInternal(optind),
          startedParse(false),
          firstReorderedArgument(string_type::npos),
          opterr(opterr),
          optopt(traits_type::to_char_type(static_cast<int_type>('?'))),
          optind(optind),
          optarg(),
          has_arg(false)
    {
    }
    enum class argument
    {
        no = 0,
        required = 1,
        optional = 2
    };
    struct option
    {
        string_type name;
        argument has_arg;
        int_type *flag;
        int_type val;
    };
private:
    static bool eq_char(char_type a, char b)
    {
        return traits_type::eq(a, traits_type::to_char_type(static_cast<int_type>(b)));
    }
    static string_type createMessageString(const char *msg)
    {
        string_type retval;
        std::size_t length = std::char_traits<char>::length(msg);
        retval.reserve(length);

        for(std::size_t i = 0; i < length; i++)
        {
            retval.push_back(traits_type::to_char_type(static_cast<int_type>(msg[i])));
        }

        return std::move(retval);
    }
    static string_type createMessageString(char_type ch)
    {
        return string_type(1, ch);
    }
    static string_type createMessageString(string_type str)
    {
        return std::move(str);
    }
    template <typename Arg1, typename ...Args>
    static string_type createMessageString(Arg1 &&arg1, const char *msg, Args &&...args)
    {
        string_type retval = createMessageString(std::forward<Arg1>(arg1));
        std::size_t length = std::char_traits<char>::length(msg);
        retval.reserve(retval.size() + length);

        for(std::size_t i = 0; i < length; i++)
        {
            retval.push_back(traits_type::to_char_type(static_cast<int_type>(msg[i])));
        }

        return createMessageString(std::move(retval), std::forward<Args>(args)...);
    }
    template <typename Arg1, typename ...Args>
    static string_type createMessageString(Arg1 &&arg1, char_type ch, Args &&...args)
    {
        string_type retval = createMessageString(std::forward<Arg1>(arg1));
        retval.push_back(ch);
        return createMessageString(std::move(retval), std::forward<Args>(args)...);
    }
    template <typename Arg1, typename ...Args>
    static string_type createMessageString(Arg1 &&arg1, const string_type &msg, Args &&...args)
    {
        string_type retval = createMessageString(std::forward<Arg1>(arg1));
        retval += msg;
        return createMessageString(std::move(retval), std::forward<Args>(args)...);
    }
    int_type getopt_engine(std::vector<string_type> &args, string_type options, const std::vector<option> &longopts, std::size_t *longindex)
    {
        if(!startedParse)
        {
            startedParse = true;
            optindInternal = optind;
            firstReorderedArgument = string_type::npos;
        }

        optarg.clear();
        has_arg = false;
        optopt = static_cast<int_type>(0);

        bool isPosixMode = false;
        bool useInOrder = false;
        bool useColonForMissingArgument = false;
        bool displayErrorMessages = true;

        if(options.size() > 0 && eq_char(options.front(), '+'))
        {
            isPosixMode = true;
            options.erase(0, 1);
        }
        else if(options.size() > 0 && eq_char(options.front(), '-'))
        {
            useInOrder = true;
            options.erase(0, 1);
        }

        if(options.size() > 0 && eq_char(options.front(), ':'))
        {
            useColonForMissingArgument = true;
            displayErrorMessages = false;
            options.erase(0, 1);
        }

        while(optindInternal < args.size())
        {
            string_type arg = args[optindInternal];

            if(arg.size() >= 2 && eq_char(arg.front(), '-')) // if arg is an option
            {
                if(eq_char(arg[1], '-')) // double dash option
                {
                    if(arg.size() == 2) // -- all by it self
                    {
                        optindInternal++;
                        optind = optindInternal;
                        return end_flag();
                    }

                    const std::size_t nameStart = 2;
                    std::size_t equalPosition = arg.find_first_of(traits_type::to_char_type(static_cast<int_type>('=')), nameStart);
                    std::size_t nameLength = arg.size() - nameStart;

                    if(equalPosition != string_type::npos)
                    {
                        nameLength = equalPosition - nameStart;
                    }

                    std::size_t match = 0;
                    std::size_t matchCount = 0;

                    for(std::size_t i = 0; i < longopts.size(); i++)
                    {
                        const option &o = longopts[i];

                        if(arg.compare(nameStart, nameLength, o.name) == 0) // if perfect match
                        {
                            match = i;
                            matchCount = 1;
                            break;
                        }
                        if(arg.compare(nameStart, nameLength, o.name, 0, nameLength) == 0) // if prefix match
                        {
                            match = i;
                            matchCount++;
                        }
                    }

                    if(matchCount > 1)
                    {
                        if(displayErrorMessages && opterr)
                        {
                            string_type msg = createMessageString("option '", arg, "' is ambiguous; possibilities:");
                            for(const option &o : longopts)
                            {
                                if(arg.compare(nameStart, nameLength, o.name, 0, nameLength) == 0)
                                {
                                    msg += createMessageString(" '--", o.name, "'");
                                }
                            }
                            opterr(msg);
                        }
                        optindInternal++;
                        optind = optindInternal;
                        return static_cast<int_type>('?');
                    }
                    else if(matchCount == 0)
                    {
                        if(displayErrorMessages && opterr)
                        {
                            opterr(createMessageString("unrecognized option '", arg, "'"));
                        }
                        optindInternal++;
                        optind = optindInternal;
                        return static_cast<int_type>('?');
                    }

                    const option &o = longopts[match];

                    switch(o.has_arg)
                    {
                    case argument::no:
                        if(equalPosition != string_type::npos)
                        {
                            if(displayErrorMessages && opterr)
                            {
                                opterr(createMessageString("option '--", o.name, "' doesn't allow an argument"));
                            }

                            optopt = o.val;
                            optindInternal++;
                            optind = optindInternal;
                            return static_cast<int_type>('?');
                        }

                        optindInternal++;
                        optind = optindInternal;

                        if(longindex)
                            *longindex = match;

                        if(o.flag)
                        {
                            *o.flag = o.val;
                            return static_cast<int_type>(0);
                        }

                        return o.val;

                    case argument::required:
                        if(equalPosition == string_type::npos)
                        {
                            if(optindInternal + 1 >= args.size())
                            {
                                if(displayErrorMessages && opterr)
                                {
                                    opterr(createMessageString("option '--", o.name, "' requires an argument"));
                                }

                                optindInternal++;
                                optind = optindInternal;
                                optopt = o.val;

                                if(useColonForMissingArgument)
                                    return static_cast<int_type>(':');

                                return static_cast<int_type>('?');
                            }

                            optindInternal++;
                            optarg = args[optindInternal];
                            has_arg = true;
                            optindInternal++;
                            optind = optindInternal;

                            if(longindex)
                                *longindex = match;

                            if(o.flag)
                            {
                                *o.flag = o.val;
                                return static_cast<int_type>(0);
                            }

                            return o.val;
                        }

                        optarg = arg.substr(equalPosition + 1);
                        has_arg = true;
                        optindInternal++;
                        optind = optindInternal;

                        if(longindex)
                            *longindex = match;

                        if(o.flag)
                        {
                            *o.flag = o.val;
                            return static_cast<int_type>(0);
                        }

                        return o.val;

                    default:
                        if(equalPosition != string_type::npos)
                        {
                            has_arg = true;
                            optarg = arg.substr(equalPosition + 1);
                        }

                        optindInternal++;
                        optind = optindInternal;

                        if(longindex)
                            *longindex = match;

                        if(o.flag)
                        {
                            *o.flag = o.val;
                            return static_cast<int_type>(0);
                        }

                        return o.val;
                    }
                }
                std::size_t optionPositionInOptions = options.find_first_of(arg[1]);
                if(eq_char(arg[1], ':'))
                {
                    optionPositionInOptions = string_type::npos;
                }
                if(optionPositionInOptions == string_type::npos)
                {
                    if(displayErrorMessages && opterr)
                    {
                        opterr(createMessageString("invalid option -- '", arg[1], "'"));
                    }
                    optopt = traits_type::to_int_type(arg[1]);
                    if(arg.size() > 2)
                    {
                        args[optindInternal].erase(1, 1);
                    }
                    else
                    {
                        optindInternal++;
                    }
                    optind = optindInternal;
                    return static_cast<int_type>('?');
                }
                bool argumentRequired = false, canHaveArgument = false;
                if(optionPositionInOptions + 1 < options.size() && eq_char(options[optionPositionInOptions + 1], ':'))
                {
                    canHaveArgument = true;
                    if(optionPositionInOptions + 2 >= options.size() || !eq_char(options[optionPositionInOptions + 2], ':'))
                    {
                        argumentRequired = true;
                    }
                }
                if(!canHaveArgument)
                {
                    int_type retval = traits_type::to_int_type(arg[1]);
                    if(arg.size() > 2)
                    {
                        args[optindInternal].erase(1, 1);
                    }
                    else
                    {
                        optindInternal++;
                    }
                    optind = optindInternal;
                    return retval;
                }
                if(arg.size() > 2)
                {
                    optarg = arg.substr(2);
                    has_arg = true;
                    optindInternal++;
                    optind = optindInternal;
                    return traits_type::to_int_type(arg[1]);
                }
                if(!argumentRequired)
                {
                    optindInternal++;
                    optind = optindInternal;
                    return traits_type::to_int_type(arg[1]);
                }
                if(optindInternal + 1 >= args.size())
                {
                    if(displayErrorMessages && opterr)
                    {
                        opterr(createMessageString("option requires an argument -- '", options[optionPositionInOptions], "'"));
                    }

                    optindInternal++;
                    optind = optindInternal;
                    optopt = options[optionPositionInOptions];

                    if(useColonForMissingArgument)
                        return static_cast<int_type>(':');

                    return static_cast<int_type>('?');
                }
                optindInternal++;
                optarg = args[optindInternal];
                has_arg = true;
                optindInternal++;
                optind = optindInternal;
                return traits_type::to_int_type(arg[1]);
            }
            if(isPosixMode)
            {
                optind = optindInternal;
                return end_flag();
            }
            if(useInOrder)
            {
                optarg = arg;
                has_arg = true;
                optindInternal++;
                optind = optindInternal;
                return static_cast<int_type>(1);
            }
            std::size_t nextOptionIndex;
            for(nextOptionIndex = optindInternal + 1; nextOptionIndex < args.size(); nextOptionIndex++)
            {
                if(nextOptionIndex >= firstReorderedArgument)
                {
                    nextOptionIndex = args.size();
                    break;
                }
                if(args[nextOptionIndex].size() >= 2 && eq_char(args[nextOptionIndex].front(), '-'))
                {
                    if(firstReorderedArgument == string_type::npos)
                    {
                        firstReorderedArgument = args.size();
                    }
                    std::rotate(args.begin() + optindInternal, args.begin() + nextOptionIndex, args.begin() + firstReorderedArgument);
                    firstReorderedArgument -= nextOptionIndex - optindInternal;
                    break;
                }
            }
            if(nextOptionIndex >= args.size())
            {
                break;
            }
        }
        if(firstReorderedArgument != string_type::npos && firstReorderedArgument != args.size())
        {
            std::rotate(args.begin() + optindInternal, args.begin() + firstReorderedArgument, args.end());
        }
        optind = optindInternal;
        return end_flag();
    }
public:
    int_type getopt(std::vector<string_type> &args, string_type options)
    {
        return getopt_engine(args, options, std::vector<option>(), nullptr);
    }
    int_type getopt_long(std::vector<string_type> &args, string_type options, const std::vector<option> &longopts, std::size_t *longindex)
    {
        return getopt_engine(args, options, longopts, longindex);
    }
};

typedef basic_get_option<char> get_option;
typedef basic_get_option<wchar_t> wget_option;

#endif // GET_OPTION_H_INCLUDED
