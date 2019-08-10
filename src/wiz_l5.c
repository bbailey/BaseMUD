/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@hypercube.org)                               *
 *       Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *       Brian Moore (zump@rom.org)                                        *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 **************************************************************************/

#include <string.h>
#include <stdlib.h>

#include "db.h"
#include "comm.h"
#include "interp.h"
#include "save.h"
#include "fight.h"
#include "utils.h"
#include "lookup.h"
#include "skills.h"
#include "affects.h"
#include "do_sub.h"
#include "recycle.h"
#include "act_info.h"
#include "chars.h"
#include "objs.h"
#include "rooms.h"
#include "find.h"

#include "wiz_l5.h"

/* TODO: review most of these functions and test them thoroughly. */
/* TODO: BAIL_IF() clauses. */
/* TODO: employ tables whenever possible */

/* trust levels for load and clone */
bool do_obj_load_check (CHAR_DATA * ch, OBJ_DATA * obj) {
    if (IS_TRUSTED (ch, GOD))
        return TRUE;
    if (IS_TRUSTED (ch, IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
        return TRUE;
    if (IS_TRUSTED (ch, DEMI) && obj->level <= 10 && obj->cost <= 500)
        return TRUE;
    if (IS_TRUSTED (ch, ANGEL) && obj->level <= 5 && obj->cost <= 250)
        return TRUE;
    if (IS_TRUSTED (ch, AVATAR) && obj->level == 0 && obj->cost <= 100)
        return TRUE;
    return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void do_clone_recurse (CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * clone) {
    OBJ_DATA *c_obj, *t_obj;
    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content) {
        if (do_obj_load_check (ch, c_obj)) {
            t_obj = create_object (c_obj->pIndexData, 0);
            clone_object (c_obj, t_obj);
            obj_to_obj (t_obj, clone);
            do_clone_recurse (ch, c_obj, t_obj);
        }
    }
}

/* RT nochannels command, for those spammers */
void do_nochannels (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Nochannel whom?", ch);
        return;
    }
    if ((victim = find_char_world (ch, arg)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (char_get_trust (victim) >= char_get_trust (ch)) {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOCHANNELS)) {
        REMOVE_BIT (victim->comm, COMM_NOCHANNELS);
        send_to_char ("The gods have restored your channel priviliges.\n\r",
                      victim);
        send_to_char ("NOCHANNELS removed.\n\r", ch);
        sprintf (buf, "$N restores channels to %s", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else {
        SET_BIT (victim->comm, COMM_NOCHANNELS);
        send_to_char ("The gods have revoked your channel priviliges.\n\r",
                      victim);
        send_to_char ("NOCHANNELS set.\n\r", ch);
        sprintf (buf, "$N revokes %s's channels.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
}

void do_noemote (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Noemote whom?\n\r", ch);
        return;
    }
    if ((victim = find_char_world (ch, arg)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (char_get_trust (victim) >= char_get_trust (ch)) {
        send_to_char ("You failed.\n\r", ch);
        return;
    }
    if (IS_SET (victim->comm, COMM_NOEMOTE)) {
        REMOVE_BIT (victim->comm, COMM_NOEMOTE);
        send_to_char ("You can emote again.\n\r", victim);
        send_to_char ("NOEMOTE removed.\n\r", ch);
        sprintf (buf, "$N restores emotes to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else {
        SET_BIT (victim->comm, COMM_NOEMOTE);
        send_to_char ("You can't emote!\n\r", victim);
        send_to_char ("NOEMOTE set.\n\r", ch);
        sprintf (buf, "$N revokes %s's emotes.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
}

void do_noshout (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Noshout whom?\n\r", ch);
        return;
    }
    if ((victim = find_char_world (ch, arg)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (IS_NPC (victim)) {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }
    if (char_get_trust (victim) >= char_get_trust (ch)) {
        send_to_char ("You failed.\n\r", ch);
        return;
    }
    if (IS_SET (victim->comm, COMM_NOSHOUT)) {
        REMOVE_BIT (victim->comm, COMM_NOSHOUT);
        send_to_char ("You can shout again.\n\r", victim);
        send_to_char ("NOSHOUT removed.\n\r", ch);
        sprintf (buf, "$N restores shouts to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else {
        SET_BIT (victim->comm, COMM_NOSHOUT);
        send_to_char ("You can't shout!\n\r", victim);
        send_to_char ("NOSHOUT set.\n\r", ch);
        sprintf (buf, "$N revokes %s's shouts.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
}

void do_notell (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Notell whom?", ch);
        return;
    }
    if ((victim = find_char_world (ch, arg)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (char_get_trust (victim) >= char_get_trust (ch)) {
        send_to_char ("You failed.\n\r", ch);
        return;
    }
    if (IS_SET (victim->comm, COMM_NOTELL)) {
        REMOVE_BIT (victim->comm, COMM_NOTELL);
        send_to_char ("You can tell again.\n\r", victim);
        send_to_char ("NOTELL removed.\n\r", ch);
        sprintf (buf, "$N restores tells to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else {
        SET_BIT (victim->comm, COMM_NOTELL);
        send_to_char ("You can't tell!\n\r", victim);
        send_to_char ("NOTELL set.\n\r", ch);
        sprintf (buf, "$N revokes %s's tells.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
}

void do_transfer (CHAR_DATA * ch, char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0') {
        send_to_char ("Transfer whom (and where)?\n\r", ch);
        return;
    }
    if (!str_cmp (arg1, "all")) {
        for (d = descriptor_list; d != NULL; d = d->next) {
            if (d->connected == CON_PLAYING
                && d->character != ch
                && d->character->in_room != NULL
                && char_can_see_anywhere (ch, d->character))
            {
                char buf[MAX_STRING_LENGTH];
                sprintf (buf, "%s %s", d->character->name, arg2);
                do_function (ch, &do_transfer, buf);
            }
        }
        return;
    }

    /* Thanks to Grodyn for the optional location parameter. */
    if (arg2[0] == '\0')
        location = ch->in_room;
    else {
        if ((location = find_location (ch, arg2)) == NULL) {
            send_to_char ("No such location.\n\r", ch);
            return;
        }
        if (!room_is_owner (location, ch) && room_is_private (location)
            && char_get_trust (ch) < MAX_LEVEL)
        {
            send_to_char ("That room is private right now.\n\r", ch);
            return;
        }
    }
    if ((victim = find_char_world (ch, arg1)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (victim->in_room == NULL) {
        send_to_char ("They are in limbo.\n\r", ch);
        return;
    }

    if (victim->fighting != NULL)
        stop_fighting (victim, TRUE);
    act ("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_NOTCHAR);
    char_from_room (victim);
    char_to_room (victim, location);
    act ("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_NOTCHAR);
    if (ch != victim)
        act ("$n has transferred you.", ch, NULL, victim, TO_VICT);
    do_function (victim, &do_look, "auto");
    send_to_char ("Ok.\n\r", ch);
}

void do_peace (CHAR_DATA * ch, char *argument) {
    CHAR_DATA *rch;

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
        if (rch->fighting != NULL)
            stop_fighting (rch, TRUE);
        if (IS_NPC (rch) && IS_SET (rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT (rch->act, ACT_AGGRESSIVE);
    }
    send_to_char ("Ok.\n\r", ch);
}

void do_snoop (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Snoop whom?\n\r", ch);
        return;
    }
    if ((victim = find_char_world (ch, arg)) == NULL) {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }
    if (victim->desc == NULL) {
        send_to_char ("No descriptor to snoop.\n\r", ch);
        return;
    }
    if (victim == ch) {
        send_to_char ("Cancelling all snoops.\n\r", ch);
        wiznet ("$N stops being such a snoop.",
                ch, NULL, WIZ_SNOOPS, WIZ_SECURE, char_get_trust (ch));
        for (d = descriptor_list; d != NULL; d = d->next)
            if (d->snoop_by == ch->desc)
                d->snoop_by = NULL;
        return;
    }
    if (victim->desc->snoop_by != NULL) {
        send_to_char ("Busy already.\n\r", ch);
        return;
    }
    if (!room_is_owner (victim->in_room, ch) && ch->in_room != victim->in_room
        && room_is_private (victim->in_room) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        send_to_char ("That character is in a private room.\n\r", ch);
        return;
    }
    if (char_get_trust (victim) >= char_get_trust (ch)
        || IS_SET (victim->comm, COMM_SNOOP_PROOF))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (ch->desc != NULL) {
        for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by) {
            if (d->character == victim || d->original == victim) {
                send_to_char ("No snoop loops.\n\r", ch);
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    sprintf (buf, "$N starts snooping on %s",
             (IS_NPC (ch) ? victim->short_descr : victim->name));
    wiznet (buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, char_get_trust (ch));
    send_to_char ("Ok.\n\r", ch);
}

void do_string (CHAR_DATA * ch, char *argument) {
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    smash_tilde (argument);
    argument = one_argument (argument, type);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0'
        || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  string char <name> <field> <string>\n\r", ch);
        send_to_char ("    fields: name short long desc title spec\n\r", ch);
        send_to_char ("  string obj  <name> <field> <string>\n\r", ch);
        send_to_char ("    fields: name short long extended\n\r", ch);
        return;
    }

    if (!str_prefix (type, "character") || !str_prefix (type, "mobile")) {
        if ((victim = find_char_world (ch, arg1)) == NULL) {
            send_to_char ("They aren't here.\n\r", ch);
            return;
        }

        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if (!str_prefix (arg2, "name")) {
            if (!IS_NPC (victim)) {
                send_to_char ("Not on PC's.\n\r", ch);
                return;
            }
            str_free (victim->name);
            victim->name = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "description")) {
            str_free (victim->description);
            victim->description = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "short")) {
            str_free (victim->short_descr);
            victim->short_descr = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "long")) {
            str_free (victim->long_descr);
            strcat (arg3, "\n\r");
            victim->long_descr = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "title")) {
            if (IS_NPC (victim)) {
                send_to_char ("Not on NPC's.\n\r", ch);
                return;
            }
            char_set_title (victim, arg3);
            return;
        }
        if (!str_prefix (arg2, "spec")) {
            if (!IS_NPC (victim)) {
                send_to_char ("Not on PC's.\n\r", ch);
                return;
            }
            if ((victim->spec_fun = spec_lookup_function (arg3)) == 0) {
                send_to_char ("No such spec fun.\n\r", ch);
                return;
            }
            return;
        }
    }

    if (!str_prefix (type, "object")) {
        /* string an obj */
        if ((obj = find_obj_world (ch, arg1)) == NULL) {
            send_to_char ("Nothing like that in heaven or earth.\n\r", ch);
            return;
        }
        if (!str_prefix (arg2, "name")) {
            str_free (obj->name);
            obj->name = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "short")) {
            str_free (obj->short_descr);
            obj->short_descr = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "long")) {
            str_free (obj->description);
            obj->description = str_dup (arg3);
            return;
        }
        if (!str_prefix (arg2, "ed") || !str_prefix (arg2, "extended")) {
            EXTRA_DESCR_DATA *ed;

            argument = one_argument (argument, arg3);
            if (argument == NULL) {
                send_to_char ("Syntax: oset <object> ed <keyword> <string>\n\r", ch);
                return;
            }
            strcat (argument, "\n\r");

            ed = extra_descr_new ();

            ed->keyword = str_dup (arg3);
            ed->description = str_dup (argument);
            LIST_FRONT (ed, next, obj->extra_descr);
            return;
        }
    }

    /* echo bad use message */
    do_function (ch, &do_string, "");
}

/* command that is similar to load */
void do_clone (CHAR_DATA * ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA *obj;

    rest = one_argument (argument, arg);
    if (arg[0] == '\0') {
        send_to_char ("Clone what?\n\r", ch);
        return;
    }
    if (!str_prefix (arg, "object")) {
        mob = NULL;
        obj = find_obj_here (ch, rest);
        if (obj == NULL) {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else if (!str_prefix (arg, "mobile") || !str_prefix (arg, "character")) {
        obj = NULL;
        mob = find_char_room (ch, rest);
        if (mob == NULL) {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else { /* find both */
        mob = find_char_room (ch, argument);
        obj = find_obj_here (ch, argument);
        if (mob == NULL && obj == NULL) {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }

    /* clone an object */
    if (obj != NULL) {
        OBJ_DATA *clone;
        if (!do_obj_load_check (ch, obj)) {
            send_to_char ("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_object (obj->pIndexData, 0);
        clone_object (obj, clone);
        if (obj->carried_by != NULL)
            obj_to_char (clone, ch);
        else
            obj_to_room (clone, ch->in_room);
        do_clone_recurse (ch, obj, clone);

        act ("You clone $p.", ch, clone, NULL, TO_CHAR);
        act ("$n has created $p.", ch, clone, NULL, TO_NOTCHAR);
        wiznet ("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE,
                char_get_trust (ch));
        return;
    }
    else if (mob != NULL) {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;
        char buf[MAX_STRING_LENGTH];

        if (!IS_NPC (mob)) {
            send_to_char ("You can only clone mobiles.\n\r", ch);
            return;
        }
        if ((mob->level > 20 && !IS_TRUSTED (ch, GOD))
            || (mob->level > 10 && !IS_TRUSTED (ch, IMMORTAL))
            || (mob->level > 5 && !IS_TRUSTED (ch, DEMI))
            || (mob->level > 0 && !IS_TRUSTED (ch, ANGEL))
            || !IS_TRUSTED (ch, AVATAR))
        {
            send_to_char
                ("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_mobile (mob->pIndexData);
        clone_mobile (mob, clone);
        for (obj = mob->carrying; obj != NULL; obj = obj->next_content) {
            if (do_obj_load_check (ch, obj)) {
                new_obj = create_object (obj->pIndexData, 0);
                clone_object (obj, new_obj );
                do_clone_recurse (ch, obj, new_obj );
                obj_to_char (new_obj, clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room (clone, ch->in_room);
        act ("You clone $N.", ch, NULL, clone, TO_CHAR);
        act ("$n has created $N.", ch, NULL, clone, TO_NOTCHAR);
        sprintf (buf, "$N clones %s.", clone->short_descr);
        wiznet (buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, char_get_trust (ch));
        return;
    }
}

/* RT anti-newbie code */
void do_newlock (CHAR_DATA * ch, char *argument) {
    extern bool newlock;
    newlock = !newlock;

    if (newlock) {
        wiznet ("$N locks out new characters.", ch, NULL, 0, 0, 0);
        send_to_char ("New characters have been locked out.\n\r", ch);
    }
    else {
        wiznet ("$N allows new characters back in.", ch, NULL, 0, 0, 0);
        send_to_char ("Newlock removed.\n\r", ch);
    }
}