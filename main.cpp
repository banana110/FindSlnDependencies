#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QMap>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    if(argc<2) return 0;
    QFile srcFile(argv[1]);
    QFileInfo info(argv[1]);
    QFile outFile(info.absolutePath() + "/new.sln");

    srcFile.open(QFile::ReadOnly);
    outFile.open(QFile::WriteOnly);

    QTextStream inF(&srcFile);
    QTextStream out(&outFile);
    out.setCodec("utf-8");

    QString projHead = info.absolutePath()+ "/";

    QString inStr = inF.readAll();
    QString inStr2 = inStr;
    QTextStream in1(&inStr);
    QTextStream in2(&inStr2);

    QMap<QString,QString> name2uuid;
    QString projPrefix = R"(Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = )";
    while(!in1.atEnd())
    {
        QString strLine = in1.readLine();
        if(strLine.startsWith(projPrefix))
        {
            QStringList pro = strLine.split(", ");
            QString proj = pro.at(2);
            QString projuuid;
            projuuid.append(&*(proj.begin()+1), proj.size()-2);//获取uuid
            proj = pro.at(0);
            QString projname;
            projname.append(&*(proj.begin()+projPrefix.size()+1), proj.size()-projPrefix.size()-2);//获取uuid
            name2uuid.insert(projname,projuuid);
        }
    }


    QString depbegin = "<AdditionalDependencies>";
    QString depend = "</AdditionalDependencies>";
    QString depsuffix = "_$(MYLibSuffix).lib";
    while(!in2.atEnd())
    {
        QString strLine = in2.readLine();
        out << strLine << Qt::endl;
        if(strLine.startsWith(projPrefix))
        {
            QString nextLine = in2.readLine();
            if(nextLine == "EndProject")
            {
                //没有处理过依赖  现在程序处理
                QStringList deps;

                QStringList pro = strLine.split(", ");
                QString proj = pro.at(1);
                QString projPath = projHead;
                projPath.append(&*(proj.begin()+1), proj.size()-2);

                QFile projF(projPath);
                projF.open(QFile::ReadOnly);
                QTextStream projIn(&projF);
                while(!projIn.atEnd())
                {
                    QString projLine = projIn.readLine().trimmed();
                    if(projLine.endsWith(depend))
                    {
                        QStringList depList =
                                projLine.mid(depbegin.size(),projLine.size()-depbegin.size()-depend.size())
                                .split(";");
                       for(auto depstr:depList)
                       {
                           if(depstr.endsWith(depsuffix))
                           {
                               deps.append(depstr.left(depstr.size()-depsuffix.size()));
                           }
                       }
                       break;
                    }
                }
                projF.close();
                QStringList depouts;
                if(deps.size()>0)
                {
                    for(auto depstr:deps)
                    {
                        auto it = name2uuid.find(depstr);
                        if(it!=name2uuid.end())
                        {
                            depouts.append(QString("		")
                                           + it.value() + " = "
                                           + it.value() );
                        }
                    }
                }
                if(depouts.size() >0 )
                {
                    out << "	ProjectSection(ProjectDependencies) = postProject" << Qt::endl;
                    for(auto str:depouts)
                    {
                        out << str << Qt::endl;
                    }
                    out << "	EndProjectSection"<< Qt::endl;
                }
            }
            out << nextLine << Qt::endl;
        }
    }
    outFile.close();
    return 0;
}
