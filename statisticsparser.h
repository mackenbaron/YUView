#ifndef STATISTICSPARSER_H
#define STATISTICSPARSER_H

#include <assert.h>
#include <vector>
#include <list>
#include <string>

enum statistics_t {
    arrowType = 0,
    blockType
};

struct StatisticsItem {
    statistics_t type;
    unsigned char color[4];
    unsigned char gridColor[3];
    float direction[2];
    int position[2];
    int size[2];
};

struct StatisticsRenderItem {
    int type_id;
    bool renderGrid;
    bool render;
    int alpha;
};

typedef std::list<StatisticsItem> Statistics;

// minimal matrix declaration
template <typename w_elem_type>
class matrix
{
public:
    typedef int t_Size;

    t_Size m_columns;
    t_Size m_rows;

    std::vector<w_elem_type> m_data;

    matrix( t_Size i_columns = 0, t_Size i_rows = 0 )
        : m_columns( i_columns ),
          m_rows( i_rows ),
          m_data( i_columns * i_rows )
    {
    }

    void resize( t_Size i_columns, t_Size i_rows) {
        m_data.resize( i_rows * i_columns );
        m_rows = i_rows;
        m_columns = i_columns;
    }

    w_elem_type * operator[]( t_Size i_index )
    {
        return & ( m_data[ i_index * m_rows ] );
    }
/*
    template <typename w_Type, int w_columns, int w_rows>
    matrix( const w_Type (&i_array)[w_columns][w_rows] )
        : m_columns( w_columns ),
          m_rows( w_rows ),
          m_data( & (i_array[0][0]), & (i_array[w_columns-1][w_rows]) )
    {
    }*/
};

struct VisualizationType;

class StatisticsParser
{
public:
    StatisticsParser();
    ~StatisticsParser();
    Statistics& getStatistics(int frameNumber, int type=0);
    Statistics getSimplifiedStatistics(int frameNumber, int type, int theshold, unsigned char color[3]);
    std::string getTypeName(int type);
    std::vector<int> getTypeIDs();
    bool parseFile(std::string filename);

    static Statistics emptyStats;
private:
    void reset();
    void parseCSVLine(std::vector<std::string> &record, const std::string& line, char delimiter);
    matrix<std::list<StatisticsItem> >* p_stats; // 2D array of type Statistics*
    std::vector<VisualizationType*> p_types;
};

#endif // STATISTICSPARSER_H
