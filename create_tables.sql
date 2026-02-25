
--Порода
CREATE TABLE breeds (
    breed_id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    min_height INT CHECK (min_height > 0),
    max_height INT CHECK (max_height >= min_height),
    color VARCHAR(100)
);

--Собаки
CREATE TABLE dogs (
    dog_id SERIAL PRIMARY KEY,
    breed_id INT REFERENCES breeds(breed_id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    owner VARCHAR(150) NOT NULL,
    address TEXT,
    is_alive BOOLEAN DEFAULT TRUE,
    psyche_test_score INT CHECK (psyche_test_score BETWEEN 1 AND 5)
);

--Родители
CREATE TABLE parents (
    dog_id INT PRIMARY KEY REFERENCES dogs(dog_id) ON DELETE CASCADE,
    father_id INT REFERENCES dogs(dog_id),
    mother_id INT REFERENCES dogs(dog_id),
    CHECK (dog_id <> father_id),
    CHECK (dog_id <> mother_id)
);

--Выставки
CREATE TABLE exhibitions (
    exhibition_id SERIAL PRIMARY KEY,
    dog_id INT REFERENCES dogs(dog_id) ON DELETE CASCADE,
    exhibition_date DATE NOT NULL,
    score INT CHECK (score BETWEEN 1 AND 5),
    medal VARCHAR(50)
);

--История болезней
CREATE TABLE diseases (
    disease_id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    treatment TEXT
);

--Справочник болезней
CREATE TABLE disease_history (
    history_id SERIAL PRIMARY KEY,
    dog_id INT REFERENCES dogs(dog_id) ON DELETE CASCADE,
    disease_id INT REFERENCES diseases(disease_id),
    date_start DATE NOT NULL,
    date_end DATE,
    CHECK (date_end IS NULL OR date_end >= date_start)
);

SELECT table_name
FROM information_schema.tables
WHERE table_schema = 'public';

SELECT * FROM breeds;
SELECT * FROM dogs;
SELECT * FROM parents;
SELECT * FROM exhibitions;
SELECT * FROM diseases;
SELECT * FROM disease_history;
