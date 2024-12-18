#include "common.h"

// Verifies is a set of chars is a number
int isNumber(char *s)
{

    for (int i = 0; i < strlen(s); i++)
    {
        if (!isdigit(s[i]))
            return false;
    }

    return true;
}

// Verifies start command arguments
int verifyStartCmd(char *PLID_buffer, char *max_playtime_buffer)
{
    int max_playtime;
    if (strlen(PLID_buffer) != 6 || !isNumber(PLID_buffer))
        return ERROR;
    
    if (isNumber(max_playtime_buffer))
    {
        max_playtime = atoi(max_playtime_buffer);
        if (max_playtime <= 0 || max_playtime > 600)
            return ERROR;
    }else
        return ERROR;
    return 0;
}

// Verifies try command arguments
int verifyTryCmd(char C1, char C2, char C3, char C4)
{
    const int n_colors = 6;

    int match_colors = 0;

    char colors[n_colors] = {'R', 'G', 'B', 'Y', 'O', 'P'};

    char guess_colors[COLORBUFFERSIZE] = {C1, C2, C3, C4};

    for (int i = 0; i < COLORBUFFERSIZE; i++)
        for (int j = 0; j < n_colors; j++)
            if (guess_colors[i] == colors[j])
            {
                match_colors++;
                break;
            }

    if (match_colors != 4)
        return ERROR;
    
    return 0;
}