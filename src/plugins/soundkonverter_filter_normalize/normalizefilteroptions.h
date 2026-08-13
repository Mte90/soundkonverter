
#ifndef NORMALIZEFILTEROPTIONS_H
#define NORMALIZEFILTEROPTIONS_H

#include "../../core/conversionoptions.h"


class NormalizeFilterOptions : public FilterOptions
{
public:
    NormalizeFilterOptions();
    ~NormalizeFilterOptions();

    bool equals( FilterOptions *_other ) override;
    QDomElement toXml( QDomDocument document, const QString& elementName ) const override;
    bool fromXml( QDomElement filterOptions ) override;

    FilterOptions* copy() const override;

    struct Data {
        bool normalize;
    } data;
};

#endif // NORMALIZEFILTEROPTIONS_H
