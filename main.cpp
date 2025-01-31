#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

/*
 * Matrix3f = Matrix<float, 3, 3>
 * Vector3f = Matrix<float, 3, 1>
 * SparseMatrix - разряженная матрица
 * Triplet - (x, y, z)  // кортеж на 3 элемента
 *
 * */

int                         nodesCount;  // Количество узлов
Eigen::VectorXf             nodesX;      // Вектор с х-координатой узлов
Eigen::VectorXf             nodesY;      // Вектор с y-координатой узлов
Eigen::VectorXf             loads;       // Вектор нагрузок

struct Element
{
	Eigen::Matrix<float, 3, 6> B;
	int nodesIds[3];

    /** \brief CalculateStiffnessMatrix - расчет матрицы жесткости
     * */
    void CalculateStiffnessMatrix(const Eigen::Matrix3f& D, std::vector<Eigen::Triplet<float> >& triplets)
    {
	    Eigen::Vector3f x, y;
	    x << nodesX[nodesIds[0]], nodesX[nodesIds[1]], nodesX[nodesIds[2]];
	    y << nodesY[nodesIds[0]], nodesY[nodesIds[1]], nodesY[nodesIds[2]];

	    Eigen::Matrix3f C;
	    C << Eigen::Vector3f(1.0f, 1.0f, 1.0f), x, y;

	    Eigen::Matrix3f inverse_C = C.inverse();

	    for( int i = 0; i < 3; ++i )
	    {
		    B(0, 2 * i + 0) = inverse_C(1, i);
		    B(0, 2 * i + 1) = 0.0f;
		    B(1, 2 * i + 0) = 0.0f;
		    B(1, 2 * i + 1) = inverse_C(2, i);
		    B(2, 2 * i + 0) = inverse_C(2, i);
		    B(2, 2 * i + 1) = inverse_C(1, i);
	    }
	    Eigen::Matrix<float, 6, 6> K = B.transpose() * D * B * C.determinant() / 2.0f;

	    for( int i = 0; i < 3; ++i )
	    {
		    for( int j = 0; j < 3; ++j )
		    {
			    Eigen::Triplet<float> trplt11(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 0, K(2 * i + 0, 2 * j + 0));
			    Eigen::Triplet<float> trplt12(2 * nodesIds[i] + 0, 2 * nodesIds[j] + 1, K(2 * i + 0, 2 * j + 1));
			    Eigen::Triplet<float> trplt21(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 0, K(2 * i + 1, 2 * j + 0));
			    Eigen::Triplet<float> trplt22(2 * nodesIds[i] + 1, 2 * nodesIds[j] + 1, K(2 * i + 1, 2 * j + 1));

			    triplets.push_back(trplt11);
			    triplets.push_back(trplt12);
			    triplets.push_back(trplt21);
			    triplets.push_back(trplt22);
		    }
	    }
    }

};

struct Constraint
{
	enum Type
	{
		UX = 1, UY, UXY
	};

	int node;
	Type type;
};

std::vector<Element>      elements;
std::vector<Constraint>   constraints;  // Ограничения

inline void SetConstraints( Eigen::SparseMatrix<float>::InnerIterator& it, const int& index )
{
	if( ( it.row() == index ) or ( it.col() == index ) )
		it.valueRef() = ( ( it.row() == it.col() ) ? 1.0f : 0.0f );
}

void ApplyConstraints(Eigen::SparseMatrix<float>& K, const std::vector<Constraint>& constraints)
{
	std::vector<int> indicesToConstraint;

    for( const auto& constraint : constraints )
    {
		if( ( constraint.type & Constraint::UX ) )
			indicesToConstraint.push_back( ( ( 2 * constraint.node ) + 0 ) );

		if( ( constraint.type & Constraint::UY ) )
			indicesToConstraint.push_back( ( ( 2 * constraint.node ) + 1 ) );
	}

	for( int k = 0, _K = K.outerSize(); k < _K; ++k )
	{
		for( Eigen::SparseMatrix<float>::InnerIterator it( K, k ); it; ++it )
		{
			for( const int& indice : indicesToConstraint )	
				SetConstraints( it, indice );
		}
	}
}

class FileException final : public std::exception
{
private:
    std::string emsg;

public:
    explicit FileException( const std::string& progName )
    {
        emsg = "usage: " + progName + " <input file> <output file>";
    }

    const char *what() const noexcept override { return emsg.c_str(); }
};

/** \brief Матрица упрогости по з-ну Гука
 * */
Eigen::Matrix3f readAcalcDmatrix( std::istream& in )
{
	float poissonRatio, youngModulus;  // mu - Коэффицент Пуассона; E - модуль Юнга [МПа]
	in >> poissonRatio >> youngModulus;

	Eigen::Matrix3f D;  // Матрица упрогости
	D << 1.0f,			poissonRatio,	0.0f,
		 poissonRatio,	1.0,			0.0f,
		 0.0f,			0.0f,			( ( 1.0f - poissonRatio ) / 2.0f );

	D *= ( youngModulus / ( 1.0f - pow( poissonRatio, 2.0f ) );
    return D;
}

/** \brief считываем ноды
 * */
void readNodes( std::istream& in )
{
	in >> nodesCount;
	nodesX.resize(nodesCount);
	nodesY.resize(nodesCount);

	for( int i = 0; i < nodesCount; ++i )
		in >> nodesX[i] >> nodesY[i];
}

int main(int argc, char *argv[])
{
	if ( argc != 3 )
        throw FileException( std::string( argv[0] ) );
	
	std::ifstream infile( argv[1] );
	std::ofstream outfile( argv[2] );
	
	Eigen::Matrix3f D = readAcalcDmatrix(infile);  // Матрица упрогости
    
    //// Ноды
    readNodes(in);

    //// Элементы
	int elementCount;
	infile >> elementCount;

	for (int i = 0; i < elementCount; ++i)
	{
		Element element;
		infile >> element.nodesIds[0] >> element.nodesIds[1] >> element.nodesIds[2];
		elements.push_back(element);
	}

    //// Ограничения
	int constraintCount;
	infile >> constraintCount;

	for (int i = 0; i < constraintCount; ++i)
	{
		Constraint constraint;
		int type;
		infile >> constraint.node >> type;
		constraint.type = static_cast<Constraint::Type>(type);
		constraints.push_back(constraint);
	}

    //// Нагрузки
	loads.resize(2 * nodesCount);
	loads.setZero();
	int loadsCount;

	infile >> loadsCount;

	for (int i = 0; i < loadsCount; ++i)
	{
		int node;
		float x, y;
		infile >> node >> x >> y;
		loads[2 * node + 0] = x;
		loads[2 * node + 1] = y;
	}
// Тут закончилось чтение
	std::vector<Eigen::Triplet<float> > triplets;
	for (std::vector<Element>::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		it->CalculateStiffnessMatrix(D, triplets);
	}

	Eigen::SparseMatrix<float> globalK(2 * nodesCount, 2 * nodesCount);
	globalK.setFromTriplets(triplets.begin(), triplets.end());

	ApplyConstraints(globalK, constraints);

	Eigen::SimplicialLDLT<Eigen::SparseMatrix<float> > solver(globalK);

	Eigen::VectorXf displacements = solver.solve(loads);

	outfile << displacements << std::endl;

	for (std::vector<Element>::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		Eigen::Matrix<float, 6, 1> delta;
		delta << displacements.segment<2>(2 * it->nodesIds[0]),
				 displacements.segment<2>(2 * it->nodesIds[1]),
				 displacements.segment<2>(2 * it->nodesIds[2]);

		Eigen::Vector3f sigma = D * it->B * delta;
		float sigma_mises = sqrt(sigma[0] * sigma[0] - sigma[0] * sigma[1] + sigma[1] * sigma[1] + 3.0f * sigma[2] * sigma[2]);

		outfile << sigma_mises << std::endl;
	}
	return 0;
}


